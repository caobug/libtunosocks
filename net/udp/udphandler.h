#pragma once

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>

#include <lwip/udp.h>
#include <lwip/ip.h>

#include "udp_session_map_def.h"
#include "../../utils/singleton.h"

#include "../protocol/socks5_protocol_helper.h"

#include "udp_session.h"
#include <boost/thread.hpp>

/*

 UDPSession handles udp packet and tunnels it through socks5

 it will construct and send socks5 udp packet directly without any tcp handshake,

 (some server may not support this method, use Socks2c instead)

 */
class UdpHandler : public Singleton<UdpHandler>
{

	using UDP_TIMER = boost::asio::deadline_timer;
	using ASIO_UDP = boost::asio::ip::udp;
	using TIME_S = boost::posix_time::seconds;

public:
	UdpHandler() : worker(boost::asio::make_work_guard(io_context))
	{
		boost::thread t1(boost::bind(&boost::asio::io_context::run, &this->io_context));
	}

	~UdpHandler()
	{
		//DVLOG(1) << this->DEBUG_STR("UDPSESSION die");
	}

	void Handle(ip_hdr* ip_header)
	{

		using namespace socks5;

		auto udp_header = reinterpret_cast<udp_hdr *>((char *)ip_header + 4 * (ip_header->_v_hl & 0x0f));

		//DVLOG(1)
		//	<< this->DEBUG_STR("udp packet From " + std::string(inet_ntoa(*(struct in_addr *) &ip_header->src)) + ":" +
		//		std::to_string(ntohs(udp_hdr->src)));
		//DVLOG(1) << this->DEBUG_STR("-> to " + std::string(inet_ntoa(*(struct in_addr *) &ip_header->dest)) + ":" +
		//	std::to_string(ntohs(udp_hdr->dest)));
		//DVLOG(1) << this->DEBUG_STR("packet size: " + std::to_string(ntohs(udp_hdr->len)));


		/*

		 The length of UDP_RELAY_PACKET is 10

		 need 2 bytes from the tail of ip_header to construct UDP_RELAY_PACKET

		 +------------+-----------+----------+
		 |    UDP_RELAY_PACKET    | PAYLOAD  |
		 +------------+-----------+----------+
		 |  IPHEADER  | UDPHEADER | PAYLOAD  |
		 +------------+-----------+----------+
		 |     2      |    8      | 1 to n   |
		 +------------+-----------+----------+

		 */
		auto socks5_udp_packet = (UDP_RELAY_PACKET*)((char*)udp_header - 2);

		// thus the send_length is udp length + 2
		int send_length = ntohs(udp_header->len) + 2;

		auto res = udp_session_map_.find(*udp_header);

		if (res == udp_session_map_.end())
		{

			auto new_session = boost::make_shared<UdpSession>(io_context, udp_session_map_);
			udp_session_map_.insert(std::make_pair(*udp_header, new_session));

			new_session->SetNatInfo(ip_header);
			// generate standard udp socks5 header
			Socks5ProtocolHelper::ConstructSocks5UdpPacketFromIpStringAndPort((unsigned char*)socks5_udp_packet, ip4addr_ntoa((ip4_addr_t*)&ip_header->dest), udp_header->dest);
			
			
			new_session->SendPacketToRemote((void*)socks5_udp_packet, send_length);

			new_session->Run();

		}
		else
		{

			//auto session_ptr = boost::static_pointer_cast<session>(res->second);

			//GenSocks5UdpPacket(socks5_udp_packet, ip_header->dest, udp_hdr->dest);

			//session_ptr->GetRemoteSocket().async_send_to(boost::asio::buffer(socks5_udp_packet, send_length), boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(globalconfig_.GetSocks5HostName()), globalconfig_.GetSocks5HostPort()), boost::bind(&session::handlerOnRemoteSent, session_ptr->shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, true));

		}
	}

private:
	UdpSessionMap udp_session_map_;

	boost::asio::io_context io_context;
	boost::asio::executor_work_guard<boost::asio::io_context::executor_type> worker;
		

};
