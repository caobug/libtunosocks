#include "net/socksifier.h"

#include "utils/app/app.h"
#include "lib/tunosocks.h"
int main()
{

    //App::Init();

    tunosocks_setsocks5("127.0.0.1", 5555);

	tunosocks_uninstall_driver();
    //tunosocks_start();



//    auto res = Socksifier::GetInstance()->Init();
//
//    if (res)
//    {
//        Socksifier::GetInstance()->AsyncRun();
//    }
//
//    getchar();

}

