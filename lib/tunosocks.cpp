#include <stdio.h>

#include "tunosocks.h"
#include "AAA.h"
#include "../utils/logger.h"
#include "../net/socks5server_info.h"
#include "../net/socksifier.h"

#ifdef __cplusplus
extern "C" {
#endif

// Inside this "extern C" block, I can define C functions that are able to call C++ code

static AAA *AAA_instance = NULL;

void lazyAAA() {
    if (AAA_instance == NULL) {
        AAA_instance = new AAA();
    }
}

void AAA_sayHi(const char *name) {
    lazyAAA();
    AAA_instance->sayHi(name);
    printf("logger\n");
}

void tunosocks_setsocks5(const char* ip, unsigned short port)
{
    Socks5ServerInfo::GetInstance()->Set(ip, port);
}

int tunosocks_start()
{
    Logger::GetInstance()->InitLog();
    Logger::GetInstance()->SetLevel(spdlog::level::debug);

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