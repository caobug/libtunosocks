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

	TcpSession(tcp_pcb* pcb, SessionMap& session_map, boost::asio::io_context& io_context) : remote_socket(io_context)
	{
		this->status = INIT;
		original_pcb = pcb;
		original_pcb->callback_arg = this;
        original_pcb_copy = *pcb;
		LOG_DEBUG("[{}] TcpSession init with pcb [{}]", (void*)this, (void*)original_pcb);
	}

	~TcpSession()
	{
        //original_pcb->callback_arg = nullptr;
		while (!pbuf_queue.empty())
		{
			auto front = pbuf_queue.front();
			pbuf_queue.pop();
			pbuf_free(front);
		}
		LOG_DEBUG("[{}] session die, pcb [{}]", (void*)this, (void*)original_pcb);
	}


	void SetSocks5ServerEndpoint(std::string ip, uint16_t port)
	{

		remote_ep = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip),port);

	}

	void Start()
	{
		if (!openRemoteSocket())
		{
			this->status = CLOSE;
			return;
		}

		auto self(this->shared_from_this());
		boost::asio::spawn(this->remote_socket.get_io_context(), [this, self](boost::asio::yield_context yield) {

			if (!connectToRemote(yield)) { this->status = CLOSE; return; }
			if (!handleMethodSelection(yield)) { this->status = CLOSE; return; }
			if (!handleSocks5Request(yield)) { this->status = CLOSE; return; }
			if (!handleRemoteRead(yield)) { this->status = CLOSE; return; }

		});

	}

	void Stop()
    {
	    if (status == CLOSE) return;
	    this->remote_socket.cancel();
	    this->status = CLOSE;
		original_pcb->callback_arg = nullptr;
    }

	tcp_pcb* GetPcb()
	{
		return original_pcb;
	}

    tcp_pcb GetPcbCopy()
    {
	    return original_pcb_copy;
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
		//assert(this->status != CLOSE);
		// if not connected to socks5 server return  
		if (this->status != RELAYING) return;

		while (!pbuf_queue.empty())
		{
			boost::system::error_code ec;

			auto front = pbuf_queue.front();
			pbuf_queue.pop();

			async_write(this->remote_socket, boost::asio::buffer(front->payload, front->tot_len),
				boost::bind(&TcpSession::handlerOnRemoteSend,
					shared_from_this(),
					boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, front));


		}

	}

	void handlerOnRemoteSend(const boost::system::error_code &ec, const size_t &size, pbuf* p)
	{
		if (ec)
		{
			this->status == CLOSE;
			return;
		}

		tcp_recved(original_pcb, size);
		LOG_DEBUG("send {} bytes to socks5 server", size)

		pbuf_free(p);
	}

	inline auto GetSeesionStatus()
	{
		return status;
	}


	void CheckReadFromRemoteStatus()
	{
		if (tcp_sndbuf(original_pcb) > TCP_LOCAL_RECV_BUFF_SIZE)
			should_read_from_remote = true;
		return;
	}

	void ReadFromRemote()
    {

		if (should_read_from_remote) return;

		auto self(this->shared_from_this());
		boost::asio::spawn(this->remote_socket.get_io_context(), [this, self](boost::asio::yield_context yield) {


			while (true)
			{
				// Downstream Coroutine
				boost::system::error_code ec;

				auto bytes_read = this->remote_socket.async_read_some(boost::asio::buffer(remote_recv_buff_, TCP_LOCAL_RECV_BUFF_SIZE), yield[ec]);

				if (ec)
				{
					LOG_DEBUG("[{:p}] handleRemoteRead err --> {}", (void*)this, ec.message().c_str())
						return false;
				}

				LOG_DEBUG("read {} bytes data from socks5 server", bytes_read);

				//stop reading if local have no buf
				//always check session status before call lwip tcp func
				// pcb could be closed
				if (status == CLOSE)
				{
					return false;
				}
				tcp_write(original_pcb, remote_recv_buff_, bytes_read, TCP_WRITE_FLAG_COPY);

				tcp_output(original_pcb);

				if (tcp_sndbuf(original_pcb) < TCP_LOCAL_RECV_BUFF_SIZE)
				{
					LOG_DEBUG("local have {} buf left stopped", tcp_sndbuf(original_pcb));
					should_read_from_remote = false;
					break;
				}
			}
			
		});
		

	    
    }



private:
	TcpSessionStatus status;

    tcp_pcb* original_pcb;
    tcp_pcb original_pcb_copy;

	std::queue<pbuf*> pbuf_queue;

	boost::asio::ip::tcp::endpoint remote_ep;
	boost::asio::ip::tcp::socket remote_socket;
	unsigned char remote_recv_buff_[TCP_REMOTE_RECV_BUFF_SIZE];

	bool should_read_from_remote = true;

	bool openRemoteSocket()
	{
		boost::system::error_code ec;

		remote_socket.open(remote_ep.protocol(), ec);

		if (ec)
		{
			LOG_ERROR("err when opening remote_socket_ --> {}", ec.message().c_str())
			return false;
		}

		boost::asio::ip::tcp::acceptor::reuse_address reuse_address(true);
		boost::asio::ip::tcp::no_delay no_delay(true);

		remote_socket.set_option(reuse_address, ec);

		if (ec)
		{
			LOG_ERROR("err reuse_address remote_socket_ --> {}", ec.message().c_str())
			return false;
		}

		remote_socket.set_option(no_delay, ec);

		if (ec)
		{
			LOG_ERROR("err set_option remote_socket_ --> {}", ec.message().c_str())
			return false;
		}

		return true;
	}

	bool connectToRemote(boost::asio::yield_context& yield)
	{
		boost::system::error_code ec;
		LOG_DEBUG("[{}] connecting to --> {}:{}", (void*)this, this->remote_ep.address().to_string().c_str(), this->remote_ep.port())

		this->remote_socket.async_connect(remote_ep, yield[ec]);

		if (ec)
		{
			LOG_DEBUG("[{}] can't connect to remote --> {}",(void*)this, ec.message().c_str())
			return false;
		}
		LOG_DEBUG("[{}] connected to --> {}:{}", (void*)this, this->remote_ep.address().to_string().c_str(), this->remote_ep.port())

		return true;
	}


	bool handleMethodSelection(boost::asio::yield_context& yield)
	{
		this->status = HANDSHAKING;

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
		auto dst = *(in_addr*)&original_pcb->local_ip.addr;
		Socks5ProtocolHelper::ConstructSocks5RequestFromIpStringAndPort(remote_recv_buff_, inet_ntoa(dst), original_pcb->local_port);

		uint64_t bytes_write = async_write(this->remote_socket, boost::asio::buffer(remote_recv_buff_, 10), yield[ec]);

		if (ec)
		{
			LOG_DEBUG("[{:p}] handleMethodSelection write err --> {}", (void*)this, ec.message().c_str())
			return false;
		}

		LOG_DEBUG("send {} bytes socks5 request", bytes_write);

		uint64_t bytes_read = boost::asio::async_read(this->remote_socket, boost::asio::buffer(remote_recv_buff_, 10), yield[ec]);

		if (ec)
		{
			LOG_DEBUG("[{:p}] handleTunnelFlow readHeaderFromRemote  err --> {}", (void*)this, ec.message().c_str())
			return false;
		}

		LOG_DEBUG("recv {} bytes socks5 reply", bytes_write);

		//check reply 0x05 0x00 0x00 0x01 // little endian
		if (*(int*)remote_recv_buff_ != 16777221) return false;
		return true;
	}


	bool handleRemoteRead(boost::asio::yield_context& yield)
	{
		this->status = RELAYING;

		//send first packet(s)
		while (!pbuf_queue.empty())
		{
			boost::system::error_code ec;

			auto front = pbuf_queue.front();
			pbuf_queue.pop();
			
			assert(front->len == front->tot_len);
			auto bytes_write = async_write(this->remote_socket, boost::asio::buffer(front->payload, front->tot_len), yield[ec]);

			if (ec)
			{
				LOG_DEBUG("[{:p}] handleRemoteRead-->async_write err --> {}", (void*)this, ec.message().c_str())
				return false;
			}
			pbuf_free(front);

			//always check session status before call lwip tcp func
			// pcb could be closed
			if (status == CLOSE)
			{
				return false;
			}
			tcp_recved(original_pcb, bytes_write);

			LOG_DEBUG("send {} bytes data to socks5 server", bytes_write);

		}

		while (1)
		{
			// Downstream Coroutine
			boost::system::error_code ec;

			auto bytes_read = this->remote_socket.async_read_some(boost::asio::buffer(remote_recv_buff_, TCP_LOCAL_RECV_BUFF_SIZE), yield[ec]);

			if (ec)
			{
				LOG_DEBUG("[{:p}] handleRemoteRead err --> {}", (void*)this, ec.message().c_str())
					return false;
			}


			LOG_DEBUG("read {} bytes data from socks5 server", bytes_read);
			//always check session status before call lwip tcp func
			// pcb could be closed
			if (status == CLOSE)
			{
				return false;
			}
			tcp_write(original_pcb, remote_recv_buff_, bytes_read, 0);

			tcp_output(original_pcb);

			if (tcp_sndbuf(original_pcb) < TCP_LOCAL_RECV_BUFF_SIZE)
			{
				LOG_DEBUG("local have {} buf left stopped", tcp_sndbuf(original_pcb));
				should_read_from_remote = false;
				break;
			}
		}

	}

};