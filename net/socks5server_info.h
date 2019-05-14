#pragma once
#include "../utils/singleton.h"
#include <string>

struct Socks5ServerInfo : public Singleton<Socks5ServerInfo>
{
	std::string ip;
	uint16_t port;

	void Set(std::string ip, uint16_t port)
	{
		ip = ip;
		port = port;
	}

	auto& GetIp()
	{
		return ip;
	}

	auto& GetPort()
	{
		return port;
	}

};

