#pragma once
#include <string>
#include "../utils/singleton.h"
struct Socks5ServerInfo : public Singleton<Socks5ServerInfo>
{
	std::string ip;
	uint16_t port;

	void Set(std::string ip, uint16_t port)
	{
		ip = ip;
		port = port;
	}

	void Get(std::string ip, uint16_t port)
	{

	}

};

