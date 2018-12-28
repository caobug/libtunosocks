#pragma once

#include "../../utils/singleton.h"
#include "tcp_session_map_def.h"
#include "tcp_session.h"
#include <boost/make_shared.hpp>
#include <boost/thread.hpp>

class TcpHandler : public Singleton<TcpHandler>
{
	


public:

	TcpHandler() : worker(boost::asio::make_work_guard(io_context))
	{
		boost::thread t1(boost::bind(&boost::asio::io_context::run, &this->io_context));
	}

	void Handle(tcp_pcb *pcb)
	{
		auto res = session_map.find(*pcb);

		assert(res == session_map.end());

		auto new_session = boost::make_shared<TcpSession>(pcb, session_map, io_context);
		session_map.insert(std::make_pair(*pcb, new_session));

		new_session->Run();
	}


private:

	SessionMap session_map;

	boost::asio::io_context io_context;
	boost::asio::executor_work_guard<boost::asio::io_context::executor_type> worker;

};