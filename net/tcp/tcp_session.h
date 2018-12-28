#pragma once

#include <boost/enable_shared_from_this.hpp>
#include "tcp_session_map_def.h"
#include <lwip/tcp.h>
#include <lwip/pbuf.h>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <queue>
#include "../protocol/socks5_protocol_helper.h"
#include "../../utils/logger.h"
#include "../bufferdef.h"

enum TcpSessionStatus
{
	INIT,
	HANDSHAKING,
	RELAYING,
	CLOSE
};

class TcpSession : public boost::enable_shared_from_this<TcpSession>
{

public:

	TcpSession(tcp_pcb* pcb, SessionMap& session_map, boost::asio::io_context& io_context) : session_map_ref(session_map), remote_socket(io_context)
	{
		original_pcb = *pcb;
	}

	~TcpSession()
	{
		LOG_DEBUG("session die!");
	}

	void Start()
	{
		if (!setNoDelay()) return;

		auto self(this->shared_from_this());
		boost::asio::spawn(this->remote_socket.get_io_context(), [this, self](boost::asio::yield_context yield) {

			if (!handleMethodSelection(yield)) return;
			if (!handleSocks5Request(yield)) return;
			if (!handleRemoteRead(yield)) return;

		});

	}

	void EnqueuePacket(pbuf* p)
	{
		pbuf_queue.push(p);
	}

	// ensure that p->len === p->tot_len
	// check session status before calling ProxyTcpPacket
	void ProxyTcpPacket(pbuf* p)
	{
		EnqueuePacket(p);

		while (!pbuf_queue.empty())
		{
			boost::system::error_code ec;

			auto front = pbuf_queue.front();
			pbuf_queue.pop();

			async_write(this->remote_socket, boost::asio::buffer(p->payload, p->tot_len),
				boost::bind(&TcpSession::handlerOnRemoteSend, 
					shared_from_this(),
					boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
			//pbuf_free(p);

		}
		
	}

	void handlerOnRemoteSend(const boost::system::error_code &ec, const size_t &size)
	{
		if (ec)
		{
			this->status == CLOSED;
			return;
		}

		LOG_DEBUG("send {} bytes to socks5 server", size)
	}

	inline auto GetSeesionStatus()
	{
		return status;
	}

private:
	TcpSessionStatus status;

	tcp_pcb original_pcb;
	SessionMap& session_map_ref;

	std::queue<pbuf*> pbuf_queue;

	boost::asio::ip::tcp::socket remote_socket;
	unsigned char remote_recv_buff_[TCP_REMOTE_RECV_BUFF_SIZE];


	bool setNoDelay()
	{
		boost::system::error_code ec;
		this->remote_socket.set_option(boost::asio::ip::tcp::no_delay(true), ec);
		if (ec)
		{
			LOG_DEBUG("[{:p}] setNoDelay err", (void*)this)
				return false;
		}
		return true;
	}

	bool handleMethodSelection(boost::asio::yield_context& yield)
	{

		boost::system::error_code ec;

		uint64_t bytes_write = async_write(this->remote_socket, boost::asio::buffer(socks5::DEFAULT_METHOD_REQUEST, 3), yield[ec]);

		if (ec)
		{
			LOG_DEBUG("[{:p}] handleMethodSelection write err --> {}", (void*)this, ec.message().c_str())
			return false;
		}

		uint64_t bytes_read = this->remote_socket.async_read_some(boost::asio::buffer(remote_recv_buff_, 2), yield[ec]);

		if (ec)
		{
			LOG_DEBUG("[{:p}] async_read_some read err --> {}", (void*)this, ec.message().c_str())
			return false;
		}

		return true;
	}



	bool handleSocks5Request(boost::asio::yield_context& yield)
	{

		boost::system::error_code ec;
		auto dst = *(in_addr*)&original_pcb.local_ip.addr;
		Socks5ProtocolHelper::ConstructSocks5RequestFromIpStringAndPort(remote_recv_buff_, inet_ntoa(dst), original_pcb.local_port);

		uint64_t bytes_write = async_write(this->remote_socket, boost::asio::buffer(remote_recv_buff_, 10), yield[ec]);

		if (ec)
		{
			LOG_DEBUG("[{:p}] handleMethodSelection write err --> {}", (void*)this, ec.message().c_str())
			return false;
		}

	}

	// we send 
	bool handleRemoteRead(boost::asio::yield_context& yield)
	{

		// Down Coroutine
		boost::system::error_code ec;

		while (1)
		{
			
		}

	}

};