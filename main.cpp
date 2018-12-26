#include "net/socksifier.h"

int main()
{


    auto res = Socksifier::GetInstance()->Init();

    if (res)
    {
        Socksifier::GetInstance()->AsyncRun();

//        sleep(3);
//
//        Socksifier::GetInstance()->Stop();
//
//        sleep(3);
//
//        Socksifier::GetInstance()->Restart();

    }

    getchar();

}

