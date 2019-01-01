#pragma once

#include <lwip/err.h>
#include <lwip/netif.h>
#include <lwip/ip.h>

#include <lwip/pbuf.h>
#include "../tuntap/tuntaphelper.h"


err_t netif_output_func(struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr) {


#ifdef __APPLE__
#define TUN_IP_OFFSET 4
#define ADD_TUN_IP_HDR 	((uint32_t*)dechained_p->payload)[0] = 33554432;
#else
#define TUN_IP_OFFSET 0
#define ADD_TUN_IP_HDR
#endif

	auto dechained_p = pbuf_alloc(PBUF_IP, p->tot_len + TUN_IP_OFFSET, PBUF_RAM);

    auto pp = p;
    int offset = 0;
    while (pp)
    {
        if (ERR_OK != pbuf_take_at(dechained_p, pp->payload, pp->len, TUN_IP_OFFSET + offset)) {
            return ERR_BUF;
        }
        offset += pp->len;
        pp = pp->next;
    }

	ADD_TUN_IP_HDR

	assert(dechained_p->len == dechained_p->tot_len);

	TuntapHelper::GetInstance()->Inject(dechained_p);

    return ERR_OK;



}
err_t netif_input_func(struct pbuf *p, struct netif *inp)
{
    return ip_input(p, inp);
}

err_t init_fn(struct netif *netif) {

    netif->name[0] = '2';
    netif->name[1] = '3';

    netif->mtu = 1500;
    netif->output = netif_output_func;

    return ERR_OK;
}





