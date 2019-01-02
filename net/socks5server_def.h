#pragma once
#include <string>

struct Socks5ServerInfo
{
	std::string ip;
	uint16_t port;

};

static Socks5ServerInfo socks5_info;