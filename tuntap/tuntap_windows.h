#pragma once

#include "singleton.h"

#include "ituntap.h"

#include <string>

class TuntapWindows : public ITuntap, public Singleton<TuntapWindows>
{
public:

	virtual bool Open();

	virtual bool Close();

protected:

	static std::wstring GetTunDeviceID();
	static std::wstring GetDeviceName(std::wstring tun_guid);

private:

	bool need_restart = false;

};

