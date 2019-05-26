#pragma once

#include <lwip/err.h>
#include <lwip/tcp.h>

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
err_t tcp_recv_func(void *arg, struct tcp_pcb *tpcb, pbuf *p, err_t err);

//call when packet send to local and recv ack
err_t tcp_sent_func(void *arg, struct tcp_pcb *tpcb, u16_t len);

void tcp_err_func(void *arg, err_t err);

err_t listener_accept_func(void *arg, struct tcp_pcb *newpcb, err_t err);