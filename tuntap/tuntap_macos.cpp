#ifdef __APPLE__

#include "tuntap_macos.h"

#include <sys/kern_control.h>
#include <sys/sys_domain.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if_utun.h>

static std::string RunShell(std::string shell_str)
{

    FILE* fp = popen(shell_str.c_str(), "r");
    if(!fp) {
        perror("popen err\n");
        exit(EXIT_FAILURE);
    }
    char buf[512] = {0};
    fgets(buf, sizeof(buf) - 1, fp);
    pclose(fp);

    return std::string(buf);
}


bool TuntapMacOS::Open()
{
    struct ctl_info ctlInfo;
    strlcpy(ctlInfo.ctl_name, UTUN_CONTROL_NAME, sizeof(ctlInfo.ctl_name));

    this->GetTunHandle() = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
    if (this->GetTunHandle() < 0) {
        perror("err creating socket");
        return false;
    }

    struct sockaddr_ctl sc;

    if (ioctl(this->GetTunHandle(), CTLIOCGINFO, &ctlInfo) == -1) {
        close(this->GetTunHandle());
        perror("ioctl err");
        return false;
    }
    //printf("ctl_info: {ctl_id: %ud, ctl_name: %s}\n",
    //       ctlInfo.ctl_id, ctlInfo.ctl_name);

    sc.sc_id = ctlInfo.ctl_id;
    sc.sc_len = sizeof(sc);
    sc.sc_family = AF_SYSTEM;
    sc.ss_sysaddr = AF_SYS_CONTROL;
    sc.sc_unit = 65;

    if (connect(this->GetTunHandle(), (struct sockaddr *)&sc, sizeof(sc)) < 0) {
        perror("can not connect socket");
        close(this->GetTunHandle());
        return false;
    }

    fcntl(this->GetTunHandle(), F_SETFL, O_NONBLOCK);

    RunShell("ifconfig utun" + std::to_string(sc.sc_unit - 1) + " 10.2.0.2/32 10.2.0.2 up");

    return true;
}

bool TuntapMacOS::Close()
{
    close(this->GetTunHandle());
    return true;
}
#endif