#pragma once

#include "../../utils/singleton.h"
#include "tcp_session_map_def.h"
#include "tcp_session.h"

class TcpHandler : public Singleton<TcpHandler>
{

public:

	TcpHandler();

	// if client close pcb we need to close the remote session which is no longer needed
	void Clear(tcp_pcb *pcb);

	//new connection if p is nullptr
	//return true if packet is handled 
	bool Handle(tcp_pcb *pcb, pbuf* p);


private:

	SessionMap session_map;

	boost::asio::io_context& io_context;

};