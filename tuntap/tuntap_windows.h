#pragma once

#include "../utils/singleton.h"

#include "ituntap.h"

#include <string>

class TuntapWindows : public ITuntap, public Singleton<TuntapWindows>
{
public:

	virtual bool Open();

	virtual bool Close();

protected:

	static std::string GetTunDeviceID();
	static std::string GetDeviceName(std::string tun_guid);

private:

	bool need_restart = false;

};

