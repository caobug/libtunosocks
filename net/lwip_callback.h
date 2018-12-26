#pragma once

#include <lwip/err.h>
#include <lwip/netif.h>
#include <lwip/ip.h>

err_t netif_output_func(struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr) {

    return ERR_OK;

}
err_t netif_input_func(struct pbuf *p, struct netif *inp) {

    return ip_input(p, inp);
}

err_t init_fn(struct netif *netif) {

    netif->name[0] = '2';
    netif->name[1] = '3';

    netif->mtu = 1500;
    netif->output = netif_output_func;

    return ERR_OK;
}





