#pragma once

#include "../utils/singleton.h"

#include "ituntap.h"

#include <string>

class TuntapMacOS : public ITuntap, public Singleton<TuntapMacOS>
{
public:

    virtual bool Open();

    virtual bool Close();


};

