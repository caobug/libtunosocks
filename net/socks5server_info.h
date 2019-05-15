#pragma once
#include "../utils/singleton.h"
#include <string>

class Socks5ServerInfo : public Singleton<Socks5ServerInfo>
{

public:
	void Set(std::string ip, uint16_t port)
	{
		this->ip = ip;
		this->port = port;
	}

	auto& GetIp()
	{
		return ip;
	}

	auto& GetPort()
	{
		return port;
	}

private:
	std::string ip;
	uint16_t port;

};

