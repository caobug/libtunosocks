#pragma once

#include <string>

#if defined(BUILD_DLL) && defined(_WIN32)
#define OS_Dll_API   __declspec( dllexport )
#else
#define OS_Dll_API
#endif

class OS_Dll_API LibTun2Socks {


public:

	static bool SetSocks5Server(std::string ip, uint16_t port);

	static bool AsyncRun();
    static bool Stop();

    static int RouteIp(std::string ip);
    static int RouteIp(uint16_t ASN);

    static int BlockIp(std::string ip);
    static int BlockIp(uint16_t ASN);

#ifdef _WIN32
	static bool InstallTunDevice();
	static bool UninstallTunDevice();
#endif

    static void ResetNetwork();

};


