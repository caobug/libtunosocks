#pragma once

#if defined(__APPLE__) || defined(__linux__)
#define TUN_HANDLE int
#include <boost/asio/posix/stream_descriptor.hpp>
#define TUN_SOCKET boost::asio::posix::stream_descriptor
#elif _WIN32
#define TUN_HANDLE HANDLE
#include <boost/asio/windows/stream_handle.hpp>
#define TUN_SOCKET boost::asio::windows::stream_handle
#endif


class ITuntap
{

public:

	virtual bool Open() = 0;

	virtual bool Close() = 0;

	auto& GetTunHandle()
	{
		return tun_handle;
	}

private:

	TUN_HANDLE tun_handle;

};