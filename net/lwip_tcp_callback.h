#pragma once

#include <lwip/err.h>
#include <lwip/tcp.h>

err_t listener_accept_func(void *arg, struct tcp_pcb *newpcb, err_t err) {

    if (err != ERR_OK) {

    }


    return ERR_OK;
}