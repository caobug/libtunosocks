#ifdef __linux__

#include "tuntap_linux.h"

#include <sys/kern_control.h>
#include <sys/sys_domain.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if_utun.h>

static std::string RunShell(std::string shell_str) {

    FILE *fp = popen(shell_str.c_str(), "r");
    if (!fp) {
        perror("popen err\n");
        exit(EXIT_FAILURE);
    }
    char buf[512] = {0};
    fgets(buf, sizeof(buf) - 1, fp);
    pclose(fp);

    return std::string(buf);
}

std::string GetDefaultGateway() {
    return RunShell("route -n|grep \"UG\"|grep -v \"UGH\"|cut -f 10 -d \" \"");
}


bool TuntapLinux::Open()
{
    struct ifreq ifr;
    int fd;
    int res;

    if ((fd = open("/dev/net/tun",O_RDWR)) < 0)
    {
        printf("can't open tun device\n");
        return false;
    }

    bzero(&ifr,sizeof(ifr));

    ifr.ifr_flags = IFF_TUN;

    if ((res = ioctl(fd,TUNSETIFF,(void *) &ifr) < 0))
    {
        printf("ioctl tunSocket error\n");
        close(fd);
        return false;
    }

    strcpy(tunDeviceName, ifr.ifr_name);

    RunShell("ifconfig " + std::string(tunDeviceName) + " 10.2.0.2/24 up");

    return fd;

}

bool TuntapMacOS::Close()
{
    close(this->GetTunHandle());
    return true;
}
#endif