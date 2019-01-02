#pragma once

#if defined(BUILD_DLL) && defined(_WIN32)
#define OS_Dll_API   __declspec( dllexport )
#else
#ifdef _WIN32
#define OS_Dll_API   __declspec( dllimport )
#else
#define OS_Dll_API
#endif
#endif

class OS_Dll_API LibTun2Socks {


public:

    static int AsyncRun(std::string socks5_ip, uint16_t socks5_port);

    static int Stop(std::string socks5_ip, uint16_t socks5_port);

    static int RouteIp(std::string ip);
    static int RouteIp(uint16_t ASN);

    static int BlockIp(std::string ip);
    static int BlockIp(uint16_t ASN);

    static void ResetNetwork();

};


