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
	SESSION_INIT,
	SESSION_HANDSHAKING,
	SESSION_RELAYING,
	SESSION_CLOSE
};

class TcpSession : public boost::enable_shared_from_this<TcpSession>
{

public:

	TcpSession(tcp_pcb* pcb, SessionMap& session_map, boost::asio::io_context& io_context) : remote_socket(io_context)
	{
		this->status = SESSION_INIT;
		original_pcb = pcb;
		original_pcb->callback_arg = this;
        original_pcb_copy = *pcb;
		LOG_DEBUG("[{}] TcpSession init with pcb [{}]", (void*)this, (void*)original_pcb);
	}

	~TcpSession()
	{
		while (!pbuf_queue.empty())
		{
			auto front = pbuf_queue.front();
			pbuf_queue.pop();
			pbuf_free(front);
		}
		//LOG_DEBUG("[{}] session die, pcb [{}]", (void*)this, (void*)original_pcb);
	}


	void SetSocks5ServerEndpoint(std::string ip, uint16_t port)
	{
		remote_ep = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip),port);
	}

	void Start()
	{
		//return if we can't open sockset
		if (!openRemoteSocket())
		{
			this->status = SESSION_CLOSE;
			return;
		}

		auto self(this->shared_from_this());
		boost::asio::spawn(this->remote_socket.get_executor(), [this, self](boost::asio::yield_context yield) {

			if (!connectToRemote(yield)) { this->status = SESSION_CLOSE; return; }
			if (!handleMethodSelection(yield)) { this->status = SESSION_CLOSE; return; }
			if (!handleSocks5Request(yield)) { this->status = SESSION_CLOSE; return; }
			if (!handleRemoteRead(yield)) { this->status = SESSION_CLOSE; return; }

		});

	}

	//never call Stop within session, it should be controlled by lwip cb
	void Stop()
    {
	    if (status == SESSION_CLOSE) return;
	    this->remote_socket.cancel();
	    this->status = SESSION_CLOSE;
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
		//it won't enqueue infinite packet cause it's limited by the recv_wnd
		EnqueuePacket(p);
		//assert(this->status != SESSION_CLOSE);

		// if not connected to socks5 server return  
		if (this->status != SESSION_RELAYING) return;

		if (this->remote_sending) return;

		this->remote_sending = true;
		auto self(this->shared_from_this());
		boost::asio::spawn(this->remote_socket.get_executor(), [self, this](boost::asio::yield_context yield){

            while (!pbuf_queue.empty())
            {
                boost::system::error_code ec;

                auto front = pbuf_queue.front();
                auto front_for_free = pbuf_queue.front();
                pbuf_queue.pop();

                while(front)
                {
                    boost::system::error_code ec;

                    auto bytes_send = async_write(this->remote_socket, boost::asio::buffer(front->payload, front->len), yield[ec]);

                    if (ec)
                    {
                        this->status = SESSION_CLOSE;
                        return;
                    }

                    if (status == SESSION_CLOSE)
                    {
                        return;
                    }

                    tcp_recved(original_pcb, bytes_send);
                    //LOG_DEBUG("send {} bytes to socks5 server", size)

                    front = front->next;
                }

                pbuf_free(front_for_free);

            }
            this->remote_sending = false;

		});

	}

	void handlerOnRemoteSend(const boost::system::error_code &ec, const size_t &size, pbuf* p)
	{
		if (ec)
		{
			this->status = SESSION_CLOSE;
			return;
		}

		if (status == SESSION_CLOSE)
		{
			return;
		}
		tcp_recved(original_pcb, size);
		//LOG_DEBUG("send {} bytes to socks5 server", size)

		pbuf_free(p);
	}

	inline auto GetSeesionStatus()
	{
		return status;
	}


	bool IsRemoteReadable()
	{
		return tcp_sndbuf(original_pcb) > 2 * TCP_LOCAL_RECV_BUFF_SIZE;
	}


	void ReadFromRemote()
    {

		//return if ReadFromRemote is called
		if (should_read_from_remote) return;

		should_read_from_remote = true;

		auto self(this->shared_from_this());
		boost::asio::spawn(this->remote_socket.get_executor(), [this, self](boost::asio::yield_context yield) {


			while (true)
			{
				// Downstream Coroutine
				boost::system::error_code ec;

				auto bytes_read = this->remote_socket.async_read_some(boost::asio::buffer(remote_recv_buff_, TCP_LOCAL_RECV_BUFF_SIZE), yield[ec]);

				if (ec)
				{
					LOG_DEBUG("[{:p}] handleRemoteRead err --> {}", (void*)this, ec.message().c_str())
					return;
				}

				//LOG_DEBUG("read {} bytes data from socks5 server", bytes_read);

				// stop reading if local have no buf
				// always check session status before call lwip tcp func
				// pcb could be closed
				if (status == SESSION_CLOSE)
				{
					return;
				}
				tcp_write(original_pcb, remote_recv_buff_, bytes_read, 0);

				tcp_output(original_pcb);

				if (tcp_sndbuf(original_pcb) < 2 * TCP_LOCAL_RECV_BUFF_SIZE)
				{
					//LOG_DEBUG("local have {} buf left stopped", tcp_sndbuf(original_pcb));
					should_read_from_remote = false;
					return;
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

	bool remote_sending = false;

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
		this->status = SESSION_HANDSHAKING;

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

		//LOG_DEBUG("send {} bytes socks5 request", bytes_write);

		uint64_t bytes_read = boost::asio::async_read(this->remote_socket, boost::asio::buffer(remote_recv_buff_, 10), yield[ec]);

		if (ec)
		{
			LOG_DEBUG("[{:p}] handleTunnelFlow readHeaderFromRemote  err --> {}", (void*)this, ec.message().c_str())
			return false;
		}

		//LOG_DEBUG("recv {} bytes socks5 reply", bytes_write);

		//check reply 0x05 0x00 0x00 0x01 // little endian
		if (*(int*)remote_recv_buff_ != 16777221) return false;

        LOG_INFO("[tcp proxy]: {}:{}", inet_ntoa(dst), original_pcb->local_port)

        return true;
	}

	
	bool handleRemoteRead(boost::asio::yield_context& yield)
	{
		//set session status to RELAYING when socks5 handshake finish
		this->status = SESSION_RELAYING;

		//local might send some packetd while session is handshaking with socks5 server 
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

			// always check session status before call lwip tcp func
			// pcb could be closed by local
			if (status == SESSION_CLOSE)
			{
				return false;
			}
			tcp_recved(original_pcb, bytes_write);

			LOG_DEBUG("send {} bytes data to socks5 server", bytes_write);

		}

		// Downstream Coroutine
		while (1)
		{
			boost::system::error_code ec;

			auto bytes_read = this->remote_socket.async_read_some(boost::asio::buffer(remote_recv_buff_, TCP_LOCAL_RECV_BUFF_SIZE), yield[ec]);

			if (ec)
			{
				LOG_DEBUG("[{:p}] handleRemoteRead err --> {}", (void*)this, ec.message().c_str())
				return false;
			}


			//LOG_DEBUG("read {} bytes data from socks5 server", bytes_read);

			// always check session status before call lwip tcp func
			// pcb could be closed by local
			if (status == SESSION_CLOSE)
			{
				return false;
			}
			tcp_write(original_pcb, remote_recv_buff_, bytes_read, 0);

			tcp_output(original_pcb);

			/*
			 * if send buf not enough(local not send ack yet),
			 * stop reading from remote until ack receive
			 */
			if (tcp_sndbuf(original_pcb) < 2 * TCP_LOCAL_RECV_BUFF_SIZE)
			{
				//LOG_DEBUG("local have {} buf left stopped", tcp_sndbuf(original_pcb));
				should_read_from_remote = false;

				//read loop is broken
				//call ReadFromRemote() when receive ack (tcp_sent_func)
				break;
			}
		}

		return false;

	}

};