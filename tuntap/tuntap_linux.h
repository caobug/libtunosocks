#ifdef __linux__
#pragma once

#include "../utils/singleton.h"

#include "ituntap.h"

#include <string>


class TuntapLinux : public ITuntap, public Singleton<TuntapLinux> {
public:

    virtual bool Open();

    virtual bool Close();

private:
    char tunDeviceName[64];


};
#endif


