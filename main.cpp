#include "net/socksifier.h"
#include "utils/logger.h"


int main()
{
	Logger::GetInstance()->InitLog();
    Logger::GetInstance()->SetLevel(spdlog::level::debug);


    auto res = Socksifier::GetInstance()->Init();

    if (res)
    {
        Socksifier::GetInstance()->AsyncRun();
    }

    getchar();

}

