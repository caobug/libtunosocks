#include "net/socksifier.h"
#include "utils/logger.h"
#include "tuntap/tuntapinstaller.h"


int main()
{
	Logger::GetInstance()->InitLog();
	//TuntapInstaller::GetInstance()->Uninstall();

	//TuntapInstaller::GetInstance()->Install();

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

