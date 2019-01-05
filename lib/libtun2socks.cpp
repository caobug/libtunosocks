#include "libtun2socks.h"

#include "../net/socksifier.h"
#include "../net/socks5server_info.h"


bool SetSocks5Server(std::string ip, uint16_t port)
{
	Socks5ServerInfo::GetInstance()->Set(ip, port);
}
bool LibTun2Socks::AsyncRun()
{
	return 0;
}

bool LibTun2Socks::Stop() { return 0; }

int LibTun2Socks::RouteIp(std::string ip) { return 0; }
int LibTun2Socks::RouteIp(uint16_t ASN) { return 0; }

int LibTun2Socks::BlockIp(std::string ip) { return 0; }
int LibTun2Socks::BlockIp(uint16_t ASN) { return 0; }

#ifdef _WIN32
bool LibTun2Socks::InstallTunDevice() { return 0; }
bool LibTun2Socks::UninstallTunDevice() { return 0; }
#endif
void LibTun2Socks::ResetNetwork() {}