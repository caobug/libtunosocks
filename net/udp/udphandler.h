#pragma once
#include <boost/asio/io_context.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/executor_work_guard.hpp>

#include "udp_session_map_def.h"
#include "../../utils/singleton.h"

/*

 UDPSession handles udp packet and tunnels it through socks5

 it will construct and send socks5 udp packet directly without any tcp handshake,

 (some server may not support this method, use Socks2c instead)

 */
class UdpHandler : public Singleton<UdpHandler>
{


public:
	UdpHandler();

	void Handle(ip_hdr* ip_header);

private:
	UdpSessionMap udp_session_map_;

	boost::asio::io_context io_context;
	boost::asio::executor_work_guard<boost::asio::io_context::executor_type> worker;
		

};
