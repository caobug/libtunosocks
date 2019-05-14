#ifndef AAA_C_CONNECTOR_H
#define AAA_C_CONNECTOR_H

#ifdef __cplusplus
extern "C" {
#endif

void AAA_sayHi(const char *name);

void tunosocks_setsocks5(const char* ip, unsigned short port);

int  tunosocks_start();

void tunosocks_stop();

#ifdef __cplusplus
}
#endif


#endif