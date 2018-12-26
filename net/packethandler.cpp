#include "packethandler.h"
#include "lwiphelper.h"

#include <iostream>

void PacketHandler::Input(void* packet, uint64_t size)
{

    for (int i = 0; i < size; ++i) {
        printf("%x ",((unsigned char*)packet)[i]);
        fflush(stdout);
    }


#if defined(__APPLE__) || defined(__linux__)
    struct pbuf *p = pbuf_alloc(PBUF_IP, size - 4, PBUF_RAM);
    if (!packet) {
        std::cout << "pbuf_alloc err\n";
    }

    if (ERR_OK == pbuf_take(p, (char*)packet + 4, size - 4)) {
        LWIP_ASSERT("len err", p->len == size - 4);
    }
#elif _WIN32
    struct pbuf *packet = pbuf_alloc(PBUF_IP, size, PBUF_RAM);
	if (!packet) {
		LOG(FATAL) << "pbuf_alloc err\n";
	}

	if (ERR_OK == pbuf_take(packet, &local_recv_buff_[0], size)) {
		LWIP_ASSERT("len err", packet->len == size);
	}
#endif

    auto netif = LwipHelper::GetInstance()->GetNetIf();

    netif.input(p, &netif);

    printf("read %zu bytes\n",size);

}