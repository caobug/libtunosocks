#ifdef _WIN32
#include "tuntapinstaller.h"
#include <winsock2.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <boost/algorithm/string.hpp>
#pragma comment(lib, "IPHLPAPI.lib")
#include "../utils/system_exec.h"

const std::string TunTapDescription = "TAP-Windows Adapter V9";

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))


std::string getTuntapAdapterName()
{
	std::vector<std::string> strs;
	auto res = ExecAndGetRes("getmac /fo csv /v");
	boost::split(strs, res, boost::is_any_of("\n"));

	if (strs.size() > 1)
	{
		for (auto str : strs)
		{
			std::vector<std::string> adapter_list;
			boost::split(adapter_list, str, boost::is_any_of(","));
			for (int i = 0; i < adapter_list.size(); i++)
			{
				if (boost::contains(adapter_list[i], TunTapDescription))
				{
					//boost::trim_if(adapter_list[i - 1], boost::is_any_of("\""));
					return adapter_list[i - 1];
				}
			}
		}
	}

	return "";
}

void TuntapInstaller::changeAdapterName()
{
	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;
	DWORD dwRetVal = 0;
	UINT i;

	/* variables used to print DHCP time info */
	struct tm newtime;
	char buffer[32];
	errno_t error;

	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
	pAdapterInfo = (IP_ADAPTER_INFO*)MALLOC(sizeof(IP_ADAPTER_INFO));
	if (pAdapterInfo == NULL) {
		printf("Error allocating memory needed to call GetAdaptersinfo\n");
		return;
	}
	// Make an initial call to GetAdaptersInfo to get
	// the necessary size into the ulOutBufLen variable
	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
		FREE(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO*)MALLOC(ulOutBufLen);
		if (pAdapterInfo == NULL) {
			printf("Error allocating memory needed to call GetAdaptersinfo\n");
			return;
		}
	}
	bool tundevicefound = false;

	if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
		pAdapter = pAdapterInfo;
		while (pAdapter) {

			if (std::string(pAdapter->Description) == TunTapDescription)
			{
				tundevicefound = true;
				break;
			}
			pAdapter = pAdapter->Next;
		}
	}
	else {
		printf("GetAdaptersInfo failed with error: %d\n", dwRetVal);

	}

	if (tundevicefound)
	{
		std::string changeNameCmd = "netsh interface set interface name = " + getTuntapAdapterName() + " newname = \"UTUN64\" ";
		system(changeNameCmd.c_str());
	}

	if (pAdapterInfo)
		FREE(pAdapterInfo);

}





#endif
