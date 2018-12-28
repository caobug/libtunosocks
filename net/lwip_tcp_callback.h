#pragma once

#include <lwip/err.h>
#include <lwip/tcp.h>

#include "tcp/tcphandler.h"

/** Function prototype for tcp receive callback functions. Called when data has
 * been received.
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param tpcb The connection pcb which received data
 * @param p The received data (or NULL when the connection has been closed!)
 * @param err An error code if there has been an error receiving
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 */
err_t tcp_recv_func(void *arg, struct tcp_pcb *tpcb, pbuf *p, err_t err)
{
	//if closed
	if (!p) return ERR_OK;

	assert(p->len == p->tot_len);
	printf("tcp_recv_func call, read %zu bytes\n", p->tot_len);

	tcp_abort(tpcb);
	return ERR_ABRT;

	auto res = TcpHandler::GetInstance()->Handle(tpcb, p);

	// !res means packet is rejected cause session is closed
	if (!res)
	{
		//TODO
	}
	return ERR_OK;
}

void tcp_err_func(void *arg, err_t err)
{
	printf("tcp_err_func call\n");

}


err_t listener_accept_func(void *arg, struct tcp_pcb *newpcb, err_t err) {

    if (err != ERR_OK) {

    }
	auto src = *(in_addr*)&newpcb->remote_ip.addr;
	auto dst = *(in_addr*)&newpcb->local_ip.addr;
	printf("tcp accepted src: %s:%d --> dst: %s:%d\n", inet_ntoa(src), newpcb->remote_port, inet_ntoa(dst), newpcb->local_port);
	//when new connection is accepted, dispatch it to tcphandler

	newpcb->recv = tcp_recv_func;
	newpcb->errf = tcp_err_func;

    return ERR_OK;
}