#include "libtun2socks.h"




int LibTun2Socks::AsyncRun(std::string socks5_ip, uint16_t socks5_port)
{

}

int LibTun2Socks::Stop(std::string socks5_ip, uint16_t socks5_port) {}

int LibTun2Socks::RouteIp(std::string ip) {}
int LibTun2Socks::RouteIp(uint16_t ASN) {}

int LibTun2Socks::BlockIp(std::string ip) {}
int LibTun2Socks::BlockIp(uint16_t ASN) {}

#ifdef _WIN32
bool LibTun2Socks::InstallTunDevice() {}
bool LibTun2Socks::UninstallTunDevice() {}
#endif
void LibTun2Socks::ResetNetwork() {}