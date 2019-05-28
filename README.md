# libtunOsocks


libtunOsocks is a library for converting network packets from Layer 3 to 4 (OSI model), it exchanges packets between ip and socks5
, you can build transparent proxy tool across platforms easily.



## Compiling ##

#### The following libraries are required 

1.  [boost 1.70.0+](https://www.boost.org/)
2.  [spdlog 1.3.1+](https://github.com/gabime/spdlog)

##### the modified version of lwIP 2.1.2 is synthesized

Compilation:

```Shell
# Create the build directory
mkdir build
cd build

# Configure the project
cmake ../

# Compile
make -j 8
```

## Shared Library

Shared library is compiled by default

## TCP Proxy

any tcp packet libtunosocks get via tun device will be injected into lwip stack. It accepts connection with any dst and the tcp data will be forwards to socks5 server

## UDP Proxy

udp packet doesn't go through lwip stack, we have a class running on the other thread for handling udp context.

we send udp socks5 packet directly without tcp handshake because it's not necessary. so it's not compatible with classic socks5 server

## ICMP Proxy

actually libtunosocks doesn't support icmp proxy, but it will send an echo back. you could ping an ip address checking if it is routed to the tun 

## IPv6 support

we think libtunosocks should run with socks5 server(splitted version) in the same machine, so there's no plan for ipv6 support 