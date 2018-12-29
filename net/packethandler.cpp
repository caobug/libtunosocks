#include "packethandler.h"
#include "lwiphelper.h"

#include <lwip/sockets.h>
#include <lwip/ip4.h>
#include <iostream>

const int PROTO_TCP = 6;
const int PROTO_UDP = 17;
const int PROTO_ICMP = 1;

void PacketHandler::Input(void* packet, uint64_t size)
{

    //for (int i = 0; i < size; ++i) {
    //    printf("%x ",((unsigned char*)packet)[i]);
    //    fflush(stdout);
    //}
#if defined(__APPLE__) || defined(__linux__)
	auto ip_header = (ip_hdr*)((char*)packet + 4);
#elif _WIN32
	auto ip_header = (ip_hdr*)packet;
#endif

	if ((ip_header->_v_hl & 0xf0) >> 4 != 0x04) {
		return;
	}

	if (IPH_PROTO(ip_header) != PROTO_TCP
		&& IPH_PROTO(ip_header) != PROTO_UDP
		&& IPH_PROTO(ip_header) != PROTO_ICMP) return;

#if defined(__APPLE__) || defined(__linux__)
    struct pbuf *p = pbuf_alloc(PBUF_IP, size - 4, PBUF_RAM);
    if (!packet) {
        std::cout << "pbuf_alloc err\n";
    }

    if (ERR_OK == pbuf_take(p, (char*)packet + 4, size - 4)) {
        assert(p->len == size - 4);
    }

    //TODO FIX
    if (p->len == 0)
    {
        abort();
    }

#elif _WIN32
    struct pbuf *p = pbuf_alloc(PBUF_IP, size, PBUF_RAM);
	if (!packet) {
		std::cout << "pbuf_alloc err\n";
	}

	if (ERR_OK == pbuf_take(p, packet, size)) {
		LWIP_ASSERT("len err", p->len == size);
	}
#endif

    auto netif = LwipHelper::GetInstance()->GetNetIf();

    netif.input(p, &netif);

    //printf("read %zu bytes\n",size);

}