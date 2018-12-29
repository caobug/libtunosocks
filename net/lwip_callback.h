#pragma once

#include <lwip/err.h>
#include <lwip/netif.h>
#include <lwip/ip.h>

#include <lwip/pbuf.h>
#include "../tuntap/tuntaphelper.h"


err_t netif_output_func(struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr) {

#ifdef __APPLE__
    struct pbuf *packet_with_tun_header = pbuf_alloc(PBUF_IP, p->len + 4, PBUF_RAM);
    if (!packet_with_tun_header) {
        printf("pbuf_alloc err\n");
        return ERR_BUF;
    }

    if (ERR_OK != pbuf_take_at(packet_with_tun_header, p->payload, p->len, 4)) {
        return ERR_BUF;
    }
	((uint32_t*)packet_with_tun_header->payload)[0] = 33554432;

	assert(packet_with_tun_header->len == packet_with_tun_header->tot_len);

	TuntapHelper::GetInstance()->Inject(packet_with_tun_header);

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





