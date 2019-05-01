#pragma once

#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include "../bufferdef.h"
#include <lwip/pbuf.h>
#include <lwip/ip.h>
#include <lwip/udp.h>
#include <boost/bind.hpp>
#include "../../utils/logger.h"
#include "../protocol/socks5_protocol_helper.h"
#include "udp_session_map_def.h"
#include <boost/asio/spawn.hpp>

#include "../../tuntap/tuntaphelper.h"

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

	UdpSession(boost::asio::io_context& io_context, UdpSessionMap& map) : \
		session_map_(map),
		remote_socket_(io_context),
		session_timer_(io_context)
	{
		remote_socket_.open(recv_ep_.protocol());
		last_update_time_ = time(nullptr);

	}


	~UdpSession()
	{



	}


	void Run()
	{
		session_status_ = SESSION_STATUS::RELAYING;
		//this->session_timer_.expires_from_now(TIME_S(5));
		//this->session_timer_.async_wait(boost::bind(&session::handlerOnTimeUp, this->shared_from_this(), boost::asio::placeholders::error));

		auto self(this->shared_from_this());
		boost::asio::spawn(this->remote_socket_.get_executor(), [this, self](boost::asio::yield_context yield) {

			while (true)
			{
				if (!readFromRemote(yield)) { return; }
			}

		});
	
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

		auto dst = std::string(inet_ntoa(*(in_addr*)&saved_ip_header->dest));
		auto src = std::string(inet_ntoa(*(in_addr*)&saved_ip_header->src));

		// swap ip_dst && ip_src
		auto tmp_ip = saved_ip_header->dest;
		saved_ip_header->dest = saved_ip_header->src;
		saved_ip_header->src = tmp_ip;

		udp_hdr *saved_udp_header = (udp_hdr *)((char *)saved_ip_header +
			4 * (ip_header->_v_hl & 0x0f));

		// swap the port
		auto tmp_port = saved_udp_header->dest;
		saved_udp_header->dest = saved_udp_header->src;
		saved_udp_header->src = tmp_port;

		return;
	}

	void SetSocks5ServerEndpoint(std::string ip, uint16_t port)
	{
		recv_ep_ = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(ip), port);
	}

    //multiple call
	void SendPacketToRemote(void* data, size_t size)
	{
        auto p = pbuf_alloc(PBUF_RAW, size, PBUF_RAM);
        if (ERR_OK != pbuf_take(p, data, size)) {

            return;
        }

		assert(size == p->tot_len);

		this->remote_socket_.async_send_to(boost::asio::buffer(p->payload, p->tot_len), recv_ep_, boost::bind(&UdpSession::handlerOnRemoteSent, this->shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, p));

	}


	auto& GetRemoteSocket()
	{
		return remote_socket_;
	}

	auto& GetRemoteRecvBuff()
	{
		return remote_recv_buff_;
	}

	void handlerOnRemoteSent(const boost::system::error_code &ec, const size_t size, pbuf* p)
	{
		if (ec)
		{
			LOG_DEBUG("handlerOnRemoteSent error --> {}", ec.message().c_str())
				this->session_status_ = SESSION_STATUS::CLOSED;
			return;
		}
		LOG_DEBUG("handlerOnRemoteSent: send {} bytes to remote", size)

		pbuf_free(p);

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


	bool readFromRemote(boost::asio::yield_context yield)
	{
		boost::system::error_code ec;
		auto bytes_read = remote_socket_.async_receive_from(boost::asio::buffer(&remote_recv_buff_[4 + ip_udp_header_.size() - 10], UDP_REMOTE_RECV_BUFF_SIZE - 4 - 10 - ip_udp_header_.size()), recv_ep_, yield[ec]);
		
		if (ec)
		{
			LOG_DEBUG("handlerOnRemoteRead error -> {}", ec.message().c_str());
			this->session_status_ = SESSION_STATUS::CLOSED;
			return false;
		}
		//LOG_INFO("UDP read {} bytes from remote", bytes_read);

		//inject back to tun device
		/*for (int i = 0; i < bytes_read; i++)
		{
			printf("%x ", remote_recv_buff_[4 + ip_udp_header_.size() - 10 + i]);
		}
		printf("\n");*/

		// Get src ip and src port from socks5 udp reply
		std::string src_ip;
		u16_t src_port;
		auto udp_socks5_hdr = (socks5::UDP_RELAY_PACKET*)&remote_recv_buff_[4 + ip_udp_header_.size() - 10];
		Socks5ProtocolHelper::parseIpPortFromSocks5UdpPacket(udp_socks5_hdr, src_ip, src_port);

		auto src_ip_uint32 = inet_addr(src_ip.c_str());

		//return if ip err
		if (src_ip_uint32 == INADDR_NONE) return true;

		//LOG_DEBUG("[{}] udp read from ep: {}:{}", (void*)this, src_ip.c_str(), src_port);

		memcpy(&remote_recv_buff_[4], &ip_udp_header_[0], ip_udp_header_.size());

		auto ip_header = (ip_hdr*)&remote_recv_buff_[4];
		
		//debug
		//assert((ip_header->_v_hl & 0x0f) > 5);
		//set src_ip
		//auto ip_uint32 = inet_addr(src_ip.c_str());
		memcpy(&ip_header->src, &src_ip_uint32, 4);

		//udp psdheader
		auto psd_header = (udppsd_header *)&ip_header->_ttl;

		auto saved_mbz = psd_header->mbz;
		psd_header->mbz = 0x00;


		/*

		 udpl = udp header length + raw udp data length (not including psd_header itself)

		 there are 10 bytes socks5 ahead of udp data, so we got (size - 10 + 8)

		 */
		auto saved_udpl = psd_header->mbz;
		psd_header->udpl = htons(bytes_read - 10 + 8);

		auto udp_header = (udp_hdr *)((char *)ip_header + 4 * (ip_header->_v_hl & 0x0f));

		udp_header->src = lwip_ntohs(src_port);

		// same value
		udp_header->len = psd_header->udpl;

		// set 0x00 before calculating checksum
		udp_header->chksum = 0x00;


		/*

		 checksum_len includes the length of udppsd_header

		 which is sizeof(udppsd_header) = 12

		 */
		auto checksum_len = sizeof(udppsd_header) + 8 + bytes_read - 10;

		// checksum unit type is short, we might paddle 0x00(byte) at the tail if it's not aligned
		auto paddle_len = checksum_len % 2;

		if (paddle_len != 0)
		{
			*((char*)(psd_header)+checksum_len) = 0x00;
		}

		auto udp_checksum = getChecksum((unsigned short*)psd_header, checksum_len + paddle_len);
		udp_header->chksum = udp_checksum;

		//restore
		psd_header->mbz = saved_mbz;
		psd_header->udpl = saved_udpl;


		ip_header->_len = htons(bytes_read - 10 + 4 * (ip_header->_v_hl & 0x0f) + 8);
		ip_header->_ttl = 64;

		calculateIpCheckSum(ip_header);
		//LOG_DEBUG("injecting {} bytes\n", (ip_header->_v_hl & 0x0f) * 4 + 8 + bytes_read);
		
		//for (int i = 0; i < (ip_header->_v_hl & 0x0f) * 4 + 8 + bytes_read; i++)
		//{
		//	printf("%x ", remote_recv_buff_[4 + i]);
		//}
		//printf("\n");
		//

#ifdef _WIN32
		TuntapHelper::GetInstance()->Inject(&remote_recv_buff_[4], (ip_header->_v_hl & 0x0f) * 4 + 8 + bytes_read - 10);
#elif defined(__APPLE__)
		*(uint32_t*)(&remote_recv_buff_[0]) = 33554432;
		TuntapHelper::GetInstance()->Inject(&remote_recv_buff_[0], (ip_header->_v_hl & 0x0f) * 4 + 8 + bytes_read - 10 + 4);
#elif defined(__linux__)

#endif
		//tun_descriptor_.async_write_some(boost::asio::buffer(&remote_recv_buff_[4], size + 18), boost::bind(&UdpSession::handlerOnLocalSent, this->shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
		return true;
	}



private:
	SESSION_STATUS session_status_;
	UdpSessionMap& session_map_;
	boost::asio::ip::udp::socket remote_socket_;
	boost::asio::ip::udp::endpoint recv_ep_;
	
	unsigned char remote_recv_buff_[UDP_REMOTE_RECV_BUFF_SIZE];
	std::vector<char> ip_udp_header_;
	ip_hdr original_ip_header;

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

		//remote_socket_.async_receive_from(boost::asio::buffer(&remote_recv_buff_[4 + ip_udp_header_.size() - 10], UDP_REMOTE_RECV_BUFF_SIZE), recv_ep_, boost::bind(&UdpSession::handlerOnRemoteRead, this->shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));


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