#include "net/socksifier.h"
#include "utils/logger.h"
#include "tuntap/tuntapinstaller.h"


int main()
{
	Logger::GetInstance()->InitLog();
    Logger::GetInstance()->SetLevel(spdlog::level::debug);

	//if(!TuntapInstaller::GetInstance()->Find())
	//{
	//	if (!TuntapInstaller::GetInstance()->Install())
	//	{
	//		LOG_INFO("Install Tuntap Driver Err")

	//			getchar();
	//		exit(0);
	//	}
	//}

    auto res = Socksifier::GetInstance()->Init();

    if (res)
    {
        Socksifier::GetInstance()->AsyncRun();
    }

    getchar();

}

