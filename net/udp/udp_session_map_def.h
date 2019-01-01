#pragma once

#include <boost/unordered_map.hpp>

#include <lwip/udp.h>



struct UDP_HEADER_HASHER
{
	size_t operator()(udp_hdr const udp_header) const
	{
		size_t seed = 0;
		boost::hash_combine(seed, udp_header.src);
		// cone nat type
		// boost::hash_combine(seed, udp_header.dest);
		return seed;
	}

};

struct UDP_HEADER_EQUAL
{
	size_t operator()(const udp_hdr &lhs, const udp_hdr &rhs) const
	{

		return lhs.src == rhs.src;
	}

};

/*

	map that store the udp nat session

 */
class UdpSession;
using UdpSessionMap = boost::unordered_map<udp_hdr, boost::shared_ptr<UdpSession>, UDP_HEADER_HASHER, UDP_HEADER_EQUAL>;




