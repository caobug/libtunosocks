#pragma once

#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include "../bufferdef.h"
#include <lwip/ip.h>
#include <lwip/udp.h>
#include <boost/bind.hpp>
#include "../../utils/logger.h"
#include "../protocol/socks5_protocol_helper.h"

enum class SESSION_STATUS : char
{
	CLOSED = 0,
	RELAYING
};


class UdpSession : public boost::enable_shared_from_this<UdpSession>
{
	// for calculating udp checksum which might be optional
	struct udppsd_header
	{
		char mbz = 0x00; // 0x00;
		unsigned char ptcl; //protocol type
		unsigned short udpl; //TCP length
		in_addr saddr; //src_addr
		in_addr daddr; //dst_addr
	};
public:

	UdpSession(boost::asio::io_context& io_context) : \
		remote_socket_(io_context),
		session_timer_(io_context),
		remote_recv_buff_(UDP_REMOTE_RECV_BUFF_SIZE)
	{
		remote_socket_.open(recv_ep_.protocol());
		last_update_time_ = time(nullptr);

	}
	void Run()
	{
		session_status_ = SESSION_STATUS::RELAYING;
		//this->session_timer_.expires_from_now(TIME_S(5));
		//this->session_timer_.async_wait(boost::bind(&session::handlerOnTimeUp, this->shared_from_this(), boost::asio::placeholders::error));
	}

	/*

	 Save bytes of ip + udp header into ip_udp_header_
	 (the max length of ip header is 60 bytes, calculated by [4 * ip_header->ip_hl] )

	 */
	void SetNatInfo(ip_hdr* ip_header)
	{

		ip_udp_header_.resize(4 * (ip_header->_v_hl & 0x0f) + 8);
		memcpy(&ip_udp_header_[0], ip_header, 4 * (ip_header->_v_hl & 0x0f) + 8);
		auto saved_ip_header = (ip_hdr*)&ip_udp_header_[0];

		// swap ip_dst && ip_src
		auto tmp_ip = saved_ip_header->dest;
		saved_ip_header->dest = ip_header->src;
		saved_ip_header->src = tmp_ip;

		udp_hdr *saved_udp_header = (udp_hdr *)((char *)saved_ip_header +
			4 * (ip_header->_v_hl & 0x0f));
		memcpy(&original_udp_header, saved_udp_header, sizeof(udp_hdr));

		// swap the port
		auto tmp_port = saved_udp_header->dest;
		saved_udp_header->dest = saved_udp_header->src;
		saved_udp_header->src = tmp_port;

		return;
	}

	~UdpSession()
	{

	}

	auto& GetRemoteSocket()
	{
		return remote_socket_;
	}

	auto& GetRemoteRecvBuff()
	{
		return remote_recv_buff_;
	}

	void handlerOnRemoteSent(const boost::system::error_code &ec, const size_t size, bool old_session)
	{
		if (ec)
		{
			LOG_DEBUG("handlerOnRemoteSent error --> {}", ec.message().c_str())
				this->session_status_ = SESSION_STATUS::CLOSED;
			return;
		}
		LOG_DEBUG("handlerOnRemoteSent: send {} bytes to remote", size)

			/*

			 the offset of remote_recv_buff_ == 4 + ip_udp_header_.size() - 10

			 4 is the psudo header ahead of the ip header
			 10 is the udp socks5 header length


			 */
			if (old_session) return;

		remote_socket_.async_receive_from(boost::asio::buffer(&remote_recv_buff_[4 + ip_udp_header_.size() - 10], UDP_REMOTE_RECV_BUFF_SIZE), recv_ep_, boost::bind(&UdpSession::handlerOnRemoteRead, this->shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	}
	inline void calculateIpCheckSum(ip_hdr* ip_header)
	{
		ip_header->_chksum = 0x00;
		auto addr = (unsigned short*)ip_header;
		size_t count = 4 * (ip_header->_v_hl & 0x0f);

		long sum = 0;
		while (count > 1) {
			sum += *(unsigned short*)addr++;
			count -= 2;
		}
		if (count > 0) {
			char left_over[2] = { 0 };
			left_over[0] = *addr;
			sum += *(unsigned short*)left_over;
		}
		while (sum >> 16)
			sum = (sum & 0xffff) + (sum >> 16);
		ip_header->_chksum = ~sum;

		return;
	}
	void handlerOnRemoteRead(const boost::system::error_code &ec, const size_t size)
	{
		if (ec)
		{
			LOG_DEBUG("handlerOnRemoteRead error -> {}", ec.message().c_str());
			this->session_status_ = SESSION_STATUS::CLOSED;
			return;
		}
		LOG_DEBUG("read {} bytes from remote", size);

		//inject back to tun device


		// Get src ip and src port from socks5 udp reply
		std::string src_ip;
		u16_t src_port;
		auto udp_socks5_hdr = (socks5::UDP_RELAY_PACKET*)&remote_recv_buff_[4 + ip_udp_header_.size() - 10];
		Socks5ProtocolHelper::parseIpPortFromSocks5UdpPacket(udp_socks5_hdr, src_ip, src_port);



		memcpy(&remote_recv_buff_[4], &ip_udp_header_[0], ip_udp_header_.size());

		auto ip_header = (ip_hdr*)&remote_recv_buff_[4];

		//debug
		assert((ip_header->_v_hl & 0x0f) > 5);
		//set src_ip
		memcpy(&ip_header->src, &src_ip, 4);

		//udp psdheader
		auto psd_header = (udppsd_header *)&ip_header->_ttl;

		psd_header->mbz = 0x00;


		/*

		 udpl = udp header length + raw udp data length (not including psd_header itself)

		 there are 10 bytes socks5 ahead of udp data, so we got (size - 10 + 8)

		 */
		psd_header->udpl = htons(size - 10 + 8);

		auto udp_header = (udp_hdr *)((char *)ip_header + 4 * (ip_header->_v_hl & 0x0f));


		//set src_port
		memcpy(&udp_header->src, &src_port, 2);

		// same value
		udp_header->len = psd_header->udpl;

		// set 0x00 before calculating checksum
		udp_header->chksum = 0x00;


		/*

		 checksum_len includes the length of udppsd_header

		 which is sizeof(udppsd_header) = 12

		 */
		auto checksum_len = sizeof(udppsd_header) + 8 + size - 10;

		// checksum unit type is short, we might paddle 0x00(byte) at the tail if it's not aligned
		auto paddle_len = checksum_len % 2;

		if (paddle_len != 0)
		{
			*((char*)(psd_header)+checksum_len) = 0x00;
		}

		auto udp_checksum = getChecksum((unsigned short*)psd_header, checksum_len + paddle_len);
		udp_header->chksum = udp_checksum;


		ip_header->_len = htons(size - 10 + 4 * (ip_header->_v_hl & 0x0f) + 8);
		ip_header->_ttl = 64;

		calculateIpCheckSum(ip_header);
		/*LOG_DEBUG("from " + std::string(inet_ntoa(*(struct in_addr *) &ip_header->src)) + ":" +
			std::to_string(ntohs(udp_header->src)));*/
			/*
			 *
			 * on linux and bsd, we have to add psd ip header
			 *
			 * which is not needed on windows
			 *
			 */
#ifdef __APPLE__
			 // mac os
		remote_recv_buff_[0] = 0x00;
		remote_recv_buff_[1] = 0x00;
		remote_recv_buff_[2] = 0x00;
		remote_recv_buff_[3] = 0x02;
#elif __linux__
			 //linux
		remote_recv_buff_[0] = 0x00;
		remote_recv_buff_[1] = 0x00;
		remote_recv_buff_[2] = 0x08;
		remote_recv_buff_[3] = 0x00;
#elif _WIN32
			 //tun_descriptor_.async_write_some(boost::asio::buffer(&remote_recv_buff_[4], size + 18), boost::bind(&UdpSession::handlerOnLocalSent, this->shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
		return;
#endif

		//tun_descriptor_.async_write_some(boost::asio::buffer(&remote_recv_buff_[0], size + 18 + 4), boost::bind(&UdpSession::handlerOnLocalSent, this->shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
		return;
	}

private:
	SESSION_STATUS session_status_;

	boost::asio::ip::udp::socket remote_socket_;
	boost::asio::ip::udp::endpoint recv_ep_;

	std::vector<char> remote_recv_buff_;
	std::vector<char> ip_udp_header_;
	ip_hdr original_udp_header;

	boost::asio::deadline_timer session_timer_;
	time_t last_update_time_;

	void handlerOnLocalSent(const boost::system::error_code &ec, const size_t &size)
	{
		if (ec)
		{
			LOG_DEBUG("handlerOnLocalSent error --> ", ec.message().c_str())
				this->session_status_ = SESSION_STATUS::CLOSED;
			return;
		}

		//DVLOG(1) << DEBUG_STR("injected " + std::to_string(size) + " bytes into tun");

		remote_socket_.async_receive_from(boost::asio::buffer(&remote_recv_buff_[4 + ip_udp_header_.size() - 10], UDP_REMOTE_RECV_BUFF_SIZE), recv_ep_, boost::bind(&UdpSession::handlerOnRemoteRead, this->shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));


	}



	void closeRemote()
	{
		boost::system::error_code ec;
		remote_socket_.close(ec);

	}

	void cancelRemote()
	{
		boost::system::error_code ec;
		remote_socket_.cancel(ec);

	}


	void updateActivity()
	{
		last_update_time_ = time(nullptr);
	}

	bool isTimeOut()
	{
		return time(nullptr) - last_update_time_ > 5 ? true : false;
	}

	inline u_short getChecksum(unsigned short* addr, size_t count)
	{
		long sum = 0;
		while (count > 1) {
			sum += *(unsigned short*)addr++;
			count -= 2;
		}
		if (count > 0) {
			char left_over[2] = { 0 };
			left_over[0] = *addr;
			sum += *(unsigned short*)left_over;
		}
		while (sum >> 16)
			sum = (sum & 0xffff) + (sum >> 16);
		return ~sum;
	}

	void handlerOnTimeUp(const boost::system::error_code &ec)
	{
		if (ec)
		{
			//DLOG(ERROR) << DEBUG_STR("handlerOnTimeUp error -> " + ec.message());
			return;
		}

		//DVLOG(1) << DEBUG_STR("handlerOnTimeUp");

		switch (this->session_status_)
		{
		case SESSION_STATUS::RELAYING:
		{
			if (isTimeOut())
			{
				//DLOG(INFO) << DEBUG_STR("session timeout canceling");
				this->cancelRemote();
				goto NEXT_LOOP;
			}
			//DLOG(INFO) << DEBUG_STR("session relaying");
			goto NEXT_LOOP;
		}
		case SESSION_STATUS::CLOSED:
		{
			//DLOG(INFO) << DEBUG_STR("session CLOSED closing");
			this->closeRemote();
			return;

		}

		}




	NEXT_LOOP:
		//this->session_timer_.expires_from_now(boost::(5));
		//this->session_timer_.async_wait(boost::bind(&session::handlerOnTimeUp, this->shared_from_this(), boost::asio::placeholders::error));
		return;
	}


};