#include "udphandler.h"
#include <boost/asio/buffer.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/asio/ip/udp.hpp>

#include <lwip/udp.h>
#include <lwip/ip.h>

#include "udp_session.h"
#include "../socks5server_info.h"
#include "../protocol/socks5_protocol_helper.h"

UdpHandler::UdpHandler() : worker(boost::asio::make_work_guard(io_context))
{
    boost::thread t1(boost::bind(&boost::asio::io_context::run, &this->io_context));
}


void UdpHandler::Handle(ip_hdr* ip_header)
{

    using namespace socks5;

    auto udp_header = reinterpret_cast<udp_hdr *>((char *)ip_header + 4 * (ip_header->_v_hl & 0x0f));

    LOG_DEBUG("read udp len {}", ntohs(udp_header->len));
    /*

     The length of UDP_RELAY_PACKET is 10

     need 2 bytes from the tail of ip_header to construct UDP_RELAY_PACKET

     +------------+-----------+----------+
     |    UDP_RELAY_PACKET    | PAYLOAD  |
     +------------+-----------+----------+
     |  IPHEADER  | UDPHEADER | PAYLOAD  |
     +------------+-----------+----------+
     |     2      |    8      | 1 to n   |
     +------------+-----------+----------+

     */
    auto socks5_udp_packet = (UDP_RELAY_PACKET*)((char*)udp_header - 2);

    // thus the send_length is udp length + 2
    int send_length = ntohs(udp_header->len) + 2;

    auto res = udp_session_map_.find(*udp_header);

    //new session
    if (res == udp_session_map_.end())
    {

        auto new_session = boost::make_shared<UdpSession>(io_context, udp_session_map_);

        auto socks_info = Socks5ServerInfo::GetInstance();

        new_session->SetSocks5ServerEndpoint(socks_info->GetIp(), socks_info->GetPort());

        udp_session_map_.insert(std::make_pair(*udp_header, new_session));

        new_session->SetNatInfo(ip_header);
        // generate standard udp socks5 header
        //LOG_INFO("[{}] new udp send to: {}:{}", (void*)new_session.get(), ip4addr_ntoa((ip4_addr_t*)&ip_header->dest), lwip_htons(udp_header->dest));
        LOG_INFO("[udp proxy]: {}:{}", ip4addr_ntoa((ip4_addr_t*)&ip_header->dest), lwip_htons(udp_header->dest))

        Socks5ProtocolHelper::ConstructSocks5UdpPacketFromIpStringAndPort((unsigned char*)socks5_udp_packet, ip4addr_ntoa((ip4_addr_t*)&ip_header->dest), udp_header->dest);

        new_session->SendPacketToRemote((void*)socks5_udp_packet, send_length);

        new_session->Run();

    }
    else
    {

        auto session_ptr = boost::static_pointer_cast<UdpSession>(res->second);
        //LOG_INFO("[{}] old udp send to: {}:{}", (void*)session_ptr.get(), ip4addr_ntoa((ip4_addr_t*)&ip_header->dest), lwip_htons(udp_header->dest));

        Socks5ProtocolHelper::ConstructSocks5UdpPacketFromIpStringAndPort((unsigned char*)socks5_udp_packet, ip4addr_ntoa((ip4_addr_t*)&ip_header->dest), udp_header->dest);

        session_ptr->SendPacketToRemote((void*)socks5_udp_packet, send_length);

    }
}