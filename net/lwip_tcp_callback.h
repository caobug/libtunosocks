#pragma once

#include <lwip/err.h>
#include <lwip/tcp.h>

#include "tcp/tcphandler.h"

err_t tcp_recv_func(void *arg, struct tcp_pcb *tpcb, pbuf *p, err_t err)
{
	


	return ERR_OK;
}

void tcp_err_func(void *arg, err_t err)
{


}


err_t listener_accept_func(void *arg, struct tcp_pcb *newpcb, err_t err) {

    if (err != ERR_OK) {

    }

	printf("tcp accepted from %s:%d\n", inet_ntoa(*(in_addr*)&newpcb->remote_ip.addr), newpcb->remote_port);
	//when new connection is accepted, dispatch it to tcphandler

	newpcb->recv = tcp_recv_func;
	newpcb->errf = tcp_err_func;

	TcpHandler::GetInstance()->Handle(newpcb);

    return ERR_OK;
}