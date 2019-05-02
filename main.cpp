#include "net/socksifier.h"
#include "tuntap/tuntapinstaller.h"

#include "utils/app/app.h"

int main()
{

    App::Init();

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

