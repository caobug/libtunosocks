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
	if (!p)
	{
		//printf("tcp close ? p == null\n");
		assert(tpcb->state == CLOSE_WAIT);
		tcp_close(tpcb);
		TcpHandler::GetInstance()->Clear(tpcb);
		return ERR_OK;
	}

	assert(p->len == p->tot_len);
	printf("tcp_recv_func call, pcb state: %d, read %hu bytes\n", tpcb->state, p->tot_len);

	// !res means packet is rejected cause session is closed
	// life of p is Handle()'s responsibility
	auto res = TcpHandler::GetInstance()->Handle(tpcb, p);
	if (!res)
	{
		LOG_DEBUG("tcp_recv data but session closed")
		TcpHandler::GetInstance()->Clear(tpcb);
		tcp_close(tpcb);
	}
	return ERR_OK;
}

err_t tcp_sent_func(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
    auto tcp_session = (TcpSession*)tpcb->callback_arg;
    tcp_session->ReadFromRemote();
	printf("tcp_sent_func call, send %d len and get ack\n", len);
	return ERR_OK;
}


void tcp_err_func(void *arg, err_t err)
{
	/*if (err == ERR_ABRT)
		printf("tcp_err_func ERR_ABRT\n");
	else
		printf("tcp_err_func err %d \n", err);
	*/
	//assert(arg != nullptr);
	if (arg == nullptr) return;
	auto tcp_session = (TcpSession*)arg;
	auto pcb_copy = tcp_session->GetPcbCopy();
	//LOG_DEBUG("tcp_err_func err {}, pcb {}", err, (void*)tcp_session->GetPcb());
	TcpHandler::GetInstance()->Clear(&pcb_copy);


}


err_t listener_accept_func(void *arg, struct tcp_pcb *newpcb, err_t err) {

	assert(err == ERR_OK);

	auto src = std::string(inet_ntoa(*(in_addr*)&newpcb->remote_ip.addr));
	auto dst = std::string(inet_ntoa(*(in_addr*)&newpcb->local_ip.addr));

	LOG_DEBUG("tcp accepted src: {}:{} --> dst: {}:{}\n", src.c_str(), newpcb->remote_port, dst.c_str(), newpcb->local_port)

	//when new connection is accepted, dispatch it to tcphandler

	newpcb->recv = tcp_recv_func;
	newpcb->sent = tcp_sent_func;
	newpcb->errf = tcp_err_func;
    tcp_nagle_disable(newpcb);
    return ERR_OK;
}