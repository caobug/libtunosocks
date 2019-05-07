#pragma once

#include <arpa/inet.h>


class UdpFilter
{
public:

    static bool Pass(uint32_t ip_dst)
    {
        return !isMulticast(ip_dst) && !isReservedAddress(ip_dst);
    }


private:

    static bool isMulticast(uint32_t ip)
    {
        char *ip_str = (char *) &ip;

        int i = ip_str[0] & 0xFF;

        // we will check only first byte of IP
        // and if it from 224 to 239, then it can
        // represent multicast IP.
        if(i >=  224 && i <= 239){
            return true;
        }

        return false;
    }

    static bool isReservedAddress(uint32_t ip)
    {
        char *ip_str = (char *) &ip;

        int i = ip_str[0] & 0xFF;

        if(i == 10 || i == 127){
            return true;
        }
        return false;
    }

};