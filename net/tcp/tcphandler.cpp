#include "tcphandler.h"
#include <boost/make_shared.hpp>
#include "../socks5server_info.h"
#include "../socksifier.h"


TcpHandler::TcpHandler() : io_context(Socksifier::GetInstance()->GetIOContext())// : worker(boost::asio::make_work_guard(io_context))
{
}

// if client close pcb we need to close the remote session which is no longer needed
void TcpHandler::Clear(tcp_pcb *pcb)
{
    auto res = session_map.find(*pcb);

    if (res == session_map.end()) return;

    res->second->Stop();
    session_map.erase(res);
}



//new connection if p is nullptr
//return true if packet is handled
bool TcpHandler::Handle(tcp_pcb *pcb, pbuf* p)
{
    auto res = session_map.find(*pcb);

    // new session
    if (res == session_map.end())
    {
        assert(p == nullptr);

        auto new_session = boost::make_shared<TcpSession>(pcb, session_map, io_context);
        session_map.insert(std::make_pair(*pcb, new_session));

        auto socks_info = Socks5ServerInfo::GetInstance();
        new_session->SetSocks5ServerEndpoint(socks_info->GetIp(), socks_info->GetPort());
        // socks5 server not connect yet, we have to enqueue the first packet
        //if (p) new_session->EnqueuePacket(p);
        new_session->Start();
        return true;
    }

    // old session
    if (res->second->GetSeesionStatus() == SESSION_CLOSE)
    {
        //LOG_DEBUG("session closed")
        pbuf_free(p);
        return false;
    }

    res->second->ProxyTcpPacket(p);

    return true;
}