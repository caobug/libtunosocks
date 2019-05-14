#pragma once

#include "../../utils/singleton.h"
#include "tcp_session_map_def.h"
#include "tcp_session.h"
#include <boost/make_shared.hpp>
#include <boost/thread.hpp>
#include "../socks5server_info.h"

class TcpHandler : public Singleton<TcpHandler>
{
	


public:

	TcpHandler() : io_context(Socksifier::GetInstance()->GetIOContext())// : worker(boost::asio::make_work_guard(io_context))
	{
		//boost::thread t1(boost::bind(&boost::asio::io_context::run, &this->io_context));
	}

	// if client close pcb we need to close the remote session which is no longer needed
	void Clear(tcp_pcb *pcb)
	{
		auto res = session_map.find(*pcb);

		if (res == session_map.end()) return;

		res->second->Stop();
		session_map.erase(res);
	}

	//new connection if p is nullptr
	//return true if packet is handled 
	bool Handle(tcp_pcb *pcb, pbuf* p)
	{
		auto res = session_map.find(*pcb);

		// new session
		if (res == session_map.end())
		{
			assert(p == nullptr);

			auto new_session = boost::make_shared<TcpSession>(pcb, session_map, io_context);
			session_map.insert(std::make_pair(*pcb, new_session));

			auto socks_info = Socks5ServerInfo::GetInstance();
			new_session->SetSocks5ServerEndpoint(socks_info->GetIp(), socks_info->GetPort());
			// socks5 server not connect yet, we have to enqueue the first packet
			//if (p) new_session->EnqueuePacket(p);
			new_session->Start();
			return true;
		}

		// old session
		if (res->second->GetSeesionStatus() == SESSION_CLOSE)
		{
			//LOG_DEBUG("session closed")
			pbuf_free(p);
			return false;
		}

		res->second->ProxyTcpPacket(p);
		
		return true;
	}


private:

	SessionMap session_map;

	boost::asio::io_context& io_context;
	//boost::asio::executor_work_guard<boost::asio::io_context::executor_type> worker;

};