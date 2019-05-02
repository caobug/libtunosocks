#pragma once

#include "../singleton.h"

#include "port_def.h"
#include "../logger.h"

#include <boost/asio/ip/udp.hpp>


class SingleInstanceApp : public Singleton<SingleInstanceApp>
{

public:

    SingleInstanceApp() : socket(io)
    {

    }

    void Init() {


        try {
            socket.open(boost::asio::ip::udp::v4());
            socket.bind(boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), unique_libtun2socks_port));
        }catch (std::exception &e)
        {
            LOG_INFO("another libtun2socks is running")
            exit(-1);
        }

    }

private:
    boost::asio::io_context io;
    boost::asio::ip::udp::socket socket;
};
