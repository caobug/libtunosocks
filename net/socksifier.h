#pragma once
#include "../utils/singleton.h"

#include <boost/asio.hpp>


#if defined(__APPLE__)
#include "../tuntap/tuntap_macos.h"
#define TuntapDevice TuntapMacOS
#elif defined(_WIN32)
#include "../tuntap/tuntap_windows.h"
#define TuntapDevice TuntapWindows
#elif defined(__linux__)
#include "../tuntap/tuntap_linux.h"
#define TuntapDevice TuntapLinux
#else
#error "Unknow Platform"
#endif

#define TUN_DEVICE_BUFFER_SIZE 2048

class Socksifier : public Singleton<Socksifier> 
{

	using IO_Context = boost::asio::io_context;
	using Worker = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
	using Timer = boost::asio::deadline_timer;
	using Time_MS = boost::posix_time::milliseconds;

public:
	Socksifier();
	~Socksifier();


	bool Init();

	void AsyncRun();

	void Stop();

	bool Restart();

	TUN_SOCKET& GetTunSocket();

	IO_Context& GetIOContext();

private:

	IO_Context io_context_;
	Worker worker_;
	Timer timer_;
	TUN_SOCKET tun_socket_;

	unsigned char tun_recv_buff_[TUN_DEVICE_BUFFER_SIZE];

};

