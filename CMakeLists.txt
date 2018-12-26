cmake_minimum_required(VERSION 3.10)
project(libtun2socks)

set(CMAKE_CXX_STANDARD 14)

include_directories(lwip/include)

add_executable(libtun2socks
        main.cpp
        net/socksifier.cpp
        net/packethandler.cpp
        tuntap/tuntap_macos.cpp

        lwip/core/ipv4/autoip.c
        lwip/core/ipv4/dhcp.c
        lwip/core/ipv4/etharp.c
        lwip/core/ipv4/icmp.c
        lwip/core/ipv4/igmp.c
        lwip/core/ipv4/ip4.c
        lwip/core/ipv4/ip4_addr.c
        lwip/core/ipv4/ip4_frag.c


        lwip/core/altcp.c
        lwip/core/altcp_alloc.c
        lwip/core/altcp_tcp.c
        lwip/core/def.c
        lwip/core/dns.c
        lwip/core/inet_chksum.c
        lwip/core/init.c
        lwip/core/ip.c
        lwip/core/mem.c
        lwip/core/memp.c
        lwip/core/netif.c
        lwip/core/pbuf.c
        lwip/core/raw.c
        lwip/core/stats.c
        lwip/core/sys.c
        lwip/core/tcp.c
        lwip/core/tcp_in.c
        lwip/core/tcp_out.c
        lwip/core/timeouts.c
        lwip/core/udp.c


        lwip/platform/sys_arch.c
        lwip/include/lwipopts.h
        net/lwiphelper.cpp
        net/lwiphelper.h
        net/lwip_callback.h
        net/lwip_tcp_callback.h net/lwip_icmp_callback.h tuntap/tuntaphelper.cpp tuntap/tuntaphelper.h)


set_target_properties(libtun2socks PROPERTIES PREFIX "")

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.68.0 REQUIRED system  coroutine context regex thread)

if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(libtun2socks ${Boost_LIBRARIES})
endif ()
