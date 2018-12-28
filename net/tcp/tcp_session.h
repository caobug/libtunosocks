#pragma once

#include <boost/enable_shared_from_this.hpp>
#include "tcp_session_map_def.h"
#include <lwip/tcp.h>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

class TcpSession : public boost::enable_shared_from_this<TcpSession>
{

public:


	TcpSession(tcp_pcb* pcb, SessionMap& session_map, boost::asio::io_context& io_context) : session_map_ref(session_map), remote_socket(io_context)
	{
		original_pcb = *pcb;
	}

	~TcpSession()
	{
		printf("session die!\n");
	}


	void Run()
	{


	}

private:


	bool socks5Handshake()
	{
		auto self(shared_from_this());
		boost::asio::spawn([this, self](boost::asio::yield_context yield) {
		
			
		});
	}


	tcp_pcb original_pcb;
	SessionMap& session_map_ref;


	boost::asio::ip::tcp::socket remote_socket;


};