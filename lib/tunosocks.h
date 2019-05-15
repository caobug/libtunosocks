#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void tunosocks_setsocks5(const char* ip, unsigned short port);

int  tunosocks_start();

void tunosocks_stop();

#ifdef __cplusplus
}
#endif

