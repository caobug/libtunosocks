#pragma once

#include <boost/unordered_map.hpp>

#include <lwip/tcp.h>

struct TCP_PCB_HASHER
{
	size_t operator()(tcp_pcb const &tcp_pcb) const {
		size_t seed = 0;
		boost::hash_combine(seed, tcp_pcb.local_ip.addr);
		boost::hash_combine(seed, tcp_pcb.remote_ip.addr);
		boost::hash_combine(seed, tcp_pcb.local_port);
		boost::hash_combine(seed, tcp_pcb.remote_port);
		return seed;
	}
};


struct TCP_PCB_EQUAL {
	size_t operator()(const tcp_pcb &lhs, const tcp_pcb &rhs) const {
		bool flag = \
			(lhs.local_ip.addr == rhs.local_ip.addr) && \
			(lhs.remote_ip.addr == rhs.remote_ip.addr) && \
			(lhs.local_port == rhs.local_port) && \
			(lhs.remote_port == rhs.remote_port);
		return flag;
	}
};

class TcpSession;
using SessionMap = boost::unordered_map<tcp_pcb, boost::shared_ptr<TcpSession>, TCP_PCB_HASHER, TCP_PCB_EQUAL>;
