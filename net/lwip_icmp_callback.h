#pragma once
#include <lwip/ip.h>
#include <lwip/icmp.h>


u8_t icmp_recv_func(void *arg, struct raw_pcb *pcb, struct pbuf *p,
                    const ip_addr_t *addr)
{

    auto ip_header = (ip_hdr*)p->payload;

    auto icmp_header = (icmp_echo_hdr*)((char*)p->payload + (ip_header->_v_hl & 0x0f) * 4);


    //eat the packet cause we need the reply from socks5 server
    return 0;
}