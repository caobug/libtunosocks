#pragma once

#include "../utils/singleton.h"

#include <lwip/netif.h>

class LwipHelper : public Singleton<LwipHelper> {

public:

    bool Init();

    netif& GetNetIf();

private:

    struct netif tun64;
    struct ip4_addr ipaddr, netmask, gw;
    struct tcp_pcb *acceptor;
    struct raw_pcb *icmp_acceptor;

};


