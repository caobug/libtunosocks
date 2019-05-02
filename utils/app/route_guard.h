#pragma once
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "../utils/system_exec.h"

static const int check_time_s = 10;

class RouteGuard
{
public:

    RouteGuard() : timer(io)
    {

    }


    void Run()
    {
        loadRouteTable();
    }


private:

    boost::asio::io_context io;
    boost::asio::deadline_timer timer;


    void loadRouteTable()
    {
        auto res = ExecAndGetRes("netstat -nr -f inet");
    }



};