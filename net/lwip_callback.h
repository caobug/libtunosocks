#pragma once

#include <lwip/err.h>
#include <lwip/netif.h>
#include <lwip/ip.h>

#include <lwip/pbuf.h>
#include "../tuntap/tuntaphelper.h"


err_t netif_output_func(struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr) {


#ifdef __APPLE__

    // p might be seperate
    auto dechained_p = pbuf_alloc(PBUF_IP, p->tot_len + 4, PBUF_RAM);
    auto pp = p;
    int offset = 0;
    while (pp)
    {
        if (ERR_OK != pbuf_take_at(dechained_p, pp->payload, pp->len, 4 + offset)) {
            return ERR_BUF;
        }
        offset += pp->len;
        pp = pp->next;
    }

	((uint32_t*)dechained_p->payload)[0] = 33554432;

	assert(dechained_p->len == dechained_p->tot_len);

	TuntapHelper::GetInstance()->Inject(dechained_p);

    return ERR_OK;
#endif

	
	TuntapHelper::GetInstance()->Inject(p);


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





