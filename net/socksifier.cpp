#include "socksifier.h"

#include "lwiphelper.h"

#include <iostream>
#include <boost/thread.hpp>

#include <boost/bind.hpp>
#define BOOST_COROUTINES_NO_DEPRECATION_WARNING
#include <boost/asio/spawn.hpp>

#include "packethandler.h"

#define TUN_RECV_COROUTINE \
auto self(GetInstance()); \
boost::asio::spawn([this, self](boost::asio::yield_context yield) {  boost::system::error_code ec; auto packet_handler = PacketHandler::GetInstance(); \
	while (true) { auto bytes_read = tun_socket_.async_read_some(boost::asio::buffer(tun_recv_buff_, TUN_DEVICE_BUFFER_SIZE), yield[ec]); \
		if (ec) {printf("async_read_some err --> %s\n", ec.message().c_str()); return; } \
		printf("read %zu bytes from tun device\n", bytes_read); \
		packet_handler->Input(tun_recv_buff_, bytes_read); \
	}\
});

Socksifier::Socksifier() : worker_(boost::asio::make_work_guard(io_context_)), timer_(io_context_), tun_socket_(io_context_) {}
Socksifier::~Socksifier() {}

bool Socksifier::Init()
{
	if (!LwipHelper::GetInstance()->Init()) {
		std::cout << "LwipHelper init failed, exiting\n";
		return false;
	}
	if (!TuntapDevice::GetInstance()->Open()) {
		std::cout << "Run failed, exiting\n";
		return false;
	}

	boost::system::error_code ec;
	tun_socket_.assign(TuntapDevice::GetInstance()->GetTunHandle(), ec);
	if (ec) {
		std::cout << "err assigning native handle to stream descriptor\n";
		return false;
	}

	return true;
}


void Socksifier::AsyncRun()
{
	TUN_RECV_COROUTINE

	boost::thread t1(boost::bind(&boost::asio::io_context::run, &this->io_context_));

}

void Socksifier::Stop()
{
	tun_socket_.cancel();
	TuntapDevice::GetInstance()->Close();
}


bool Socksifier::Restart()
{
	if (!TuntapDevice::GetInstance()->Open()) {
		std::cout << "Run failed, exiting\n";
		return false;
	}
	TUN_RECV_COROUTINE
	return true;
}

TUN_SOCKET& Socksifier::GetTunSocket()
{
	return tun_socket_;
}