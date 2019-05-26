//
// Created by System Administrator on 2018-12-25.
//

#include "lwiphelper.h"

#include "lwip_callback.h"
#include "lwip_tcp_callback.h"
#include "lwip_icmp_callback.h"
#include <lwip/init.h>
#include <lwip/tcp.h>
#include <lwip/raw.h>

bool LwipHelper::Init()
{
    lwip_init();

    IP4_ADDR(&this->ipaddr, 10, 2, 0, 1);
    IP4_ADDR(&this->netmask, 255, 255, 255, 255);

    netif_add(&tun64, &ipaddr, &netmask, nullptr, nullptr, &init_fn, &netif_input_func);
    netif_set_default(&tun64);
    netif_set_up(&tun64);


    // tcp setup
    acceptor = tcp_new();
    tcp_nagle_disable(acceptor);
    if (ERR_OK != tcp_bind(acceptor, IP4_ADDR_ANY, 443)) {
        printf("tcp acceptor bind err\n");
        return false;
    }
    acceptor = tcp_listen(acceptor);
    tcp_accept(acceptor, listener_accept_func);


    // icmp setup
    icmp_acceptor = raw_new(IP_PROTO_ICMP);
    if (!icmp_acceptor)
    {
        printf("icmp acceptor init err\n");
        return false;
    }

    raw_recv(icmp_acceptor, icmp_recv_func, this);
    raw_bind(icmp_acceptor, IP_ADDR_ANY);


    return true;
}

netif& LwipHelper::GetNetIf()
{
    return tun64;
}