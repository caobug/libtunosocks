#pragma once

#include "../singleton.h"
#include "../logger.h"

#include "port_def.h"

#include <boost/asio/ip/udp.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/thread.hpp>

const int check_time_s = 10;

/*
 * Checker must be running
 */
class ReadStateChecker : public Singleton<ReadStateChecker>
{

public:

    ReadStateChecker() : checker_timer(io)
    {

    }

    void Run() {

        boost::asio::spawn(this->io, [this](boost::asio::yield_context yield){

            while(true)
            {

                try {
                    boost::asio::ip::udp::socket socket(io);
                    socket.open(boost::asio::ip::udp::v4());
                    socket.bind(boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), unique_libtun2socks_watcher_port));
                    LOG_INFO("unable to find watcher, exiting")
                    exit(-1);
                }catch (std::exception &e)
                {
                    boost::system::error_code ec;
                    checker_timer.expires_from_now(boost::posix_time::seconds(check_time_s));
                    checker_timer.async_wait(yield[ec]);
                    if (ec)
                    {
                        LOG_INFO("checker_timer err --> {}", ec.message())
                        exit(-1);
                    }
                    continue;
                }
            }

        });


        boost::thread t(boost::bind(&boost::asio::io_context::run, &this->io));
        t.detach();
    }

private:
    boost::asio::io_context io;
    boost::asio::deadline_timer checker_timer;
};
