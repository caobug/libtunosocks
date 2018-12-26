#pragma once
#include <lwip/ip.h>
#include <lwip/icmp.h>
#include <lwip/pbuf.h>

#include "../tuntap/tuntaphelper.h"




u8_t icmp_recv_func(void *arg, struct raw_pcb *pcb, struct pbuf *p,
                    const ip_addr_t *addr)
{

    struct pbuf *icmp_packet = pbuf_alloc(PBUF_IP, p->len + 4, PBUF_RAM);
    if (!icmp_packet) {
        printf("pbuf_alloc err\n");
        return 1;
    }

    if (ERR_OK != pbuf_take_at(icmp_packet, p->payload, p->len, 4)) {
        return 1;
    }

    ((uint32_t*)icmp_packet->payload)[0] = 2;


    printf("icmp !!!!!\n");
    for (int i = 0; i < p->tot_len; ++i) {
        printf("%x ",((unsigned char*)icmp_packet->payload)[i]);
        fflush(stdout);
    }

    assert(icmp_packet->len == icmp_packet->tot_len);

    TuntapHelper::GetInstance()->Inject(icmp_packet);

    //eat the packet cause we need the reply from socks5 server
    return 1;
}