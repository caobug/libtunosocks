#include "net/socksifier.h"
#include "utils/logger.h"
#include "tuntap/tuntapinstaller.h"


int main()
{
	Logger::GetInstance()->InitLog();
    Logger::GetInstance()->SetLevel(spdlog::level::debug);
	//TuntapInstaller::GetInstance()->Uninstall();
	if (!TuntapInstaller::GetInstance()->Find())
	{
		TuntapInstaller::GetInstance()->Install();
	}

    auto res = Socksifier::GetInstance()->Init();

    if (res)
    {
        Socksifier::GetInstance()->AsyncRun();

//        sleep(3);
////
//        Socksifier::GetInstance()->Stop();
////
//        sleep(3);
////
//        Socksifier::GetInstance()->Restart();

    }

    getchar();

}

