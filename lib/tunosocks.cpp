#include <stdio.h>

#include "tunosocks.h"
#include "../utils/logger.h"
#include "../net/socks5server_info.h"
#include "../net/socksifier.h"
#include "../tuntap/tuntapinstaller.h"

#ifdef __cplusplus
extern "C" {
#endif
	

int tunosocks_install_driver()
{
#ifdef _WIN32
	if(!TuntapInstaller::GetInstance()->Find())
	{
		if (!TuntapInstaller::GetInstance()->Install())
		{
			return -1;
		}
	}
#endif
	return 0;
}

int tunosocks_uninstall_driver()
{
#ifdef _WIN32
	if (TuntapInstaller::GetInstance()->Find())
	{
		if (!TuntapInstaller::GetInstance()->Uninstall())
		{
			return -1;
		}
	}
#endif
	return 0;
}

void tunosocks_setsocks5(const char* ip, unsigned short port)
{
    Logger::GetInstance()->InitLog();
    Logger::GetInstance()->SetLevel(spdlog::level::info);

    LOG_INFO("set socks5 server: {}:{}", ip, port)
    Socks5ServerInfo::GetInstance()->Set(ip, port);
}

int tunosocks_start()
{
    auto res = Socksifier::GetInstance()->Init();
    if (res)
    {
        Socksifier::GetInstance()->AsyncRun();
        return 0;
    }
    return -1;

}

void tunosocks_stop()
{
    Socksifier::GetInstance()->Stop();
}


#ifdef __cplusplus
}
#endif