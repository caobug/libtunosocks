#pragma once

#include "../utils/singleton.h"

class PacketHandler : public Singleton<PacketHandler>
{

public:

	void Input(void* packet, uint64_t size);

};

