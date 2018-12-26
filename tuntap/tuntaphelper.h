#pragma once

#include "../utils/singleton.h"
#include "../net/socksifier.h"
#include <lwip/pbuf.h>

#include <boost/system/error_code.hpp>
#include <boost/asio/write.hpp>
#include <boost/bind.hpp>

class TuntapHelper : public Singleton<TuntapHelper> {


public:

    void Inject(void *data, size_t size) {

        boost::asio::async_write(Socksifier::GetInstance()->GetTunSocket(),
                boost::asio::buffer(data, size),
                boost::bind(&TuntapHelper::handlerOnTunInject, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));


    }

    void Inject(pbuf* p) {

        boost::asio::async_write(Socksifier::GetInstance()->GetTunSocket(),
                                 boost::asio::buffer(p->payload, p->len),
                                 boost::bind(&TuntapHelper::handlerOnTunInjectWithPbuf, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, p));


    }

private:

    void handlerOnTunInject(const boost::system::error_code &ec, const size_t &size) {


    }

    void handlerOnTunInjectWithPbuf(const boost::system::error_code &ec, const size_t &size, pbuf* p)
    {


        if (ec)
        {
            printf("err inject %zu bytes into tun device --> %s\n", size, ec.message().c_str());
            return;
        }


        printf("inject %zu bytes into tun device\n", size);


    }

};