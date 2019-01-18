#ifdef _WIN32
#include "tuntap_windows.h"
#include "tap-windows.h"
#include <string>

std::string TuntapWindows::GetTunDeviceID() 
{
	HKEY hkey;
	auto adapterKey = TEXT("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}");

	if (ERROR_SUCCESS != RegOpenKeyExA(HKEY_LOCAL_MACHINE, adapterKey, 0, KEY_READ, &hkey)) {
		exit(-1);
	}

	DWORD subKeyNum;
	DWORD subKeyNameMaxLen;
	RegQueryInfoKey(hkey, NULL, NULL, 0, &subKeyNum, &subKeyNameMaxLen, NULL, NULL, NULL, NULL, NULL, NULL);

	auto subKeyName = new char[subKeyNameMaxLen + 1];

	for (DWORD i = 1; i < subKeyNum; i++) {
		DWORD NameNum = subKeyNameMaxLen + 1;
		int res = RegEnumKeyExA(hkey, i, subKeyName, &NameNum, NULL, NULL, NULL, NULL);

		if (res != ERROR_SUCCESS) {
			break;
		}
		//printf("%s\n", subKeyName);
		// open subkey 0000, 0001, 0002 ....

		DWORD dwKeyValueType;
		DWORD dwKeyValueDataSize = 256;

		LPBYTE lpbKeyValueData = new BYTE[256];

		HKEY key;
		std::string subkey = std::string(adapterKey) + std::string(TEXT("\\")) + std::string(subKeyName);
		res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, subkey.c_str(), 0, KEY_READ, &key);
		if (ERROR_SUCCESS != res) {
			continue;
		}


		const auto valueName = TEXT("MatchingDeviceId");
		const auto NetCfgName = TEXT("NetCfgInstanceId");
		const std::string expect_value(TEXT("tap0901"));
		res = RegQueryValueExA(key, valueName, NULL, &dwKeyValueType, lpbKeyValueData, &dwKeyValueDataSize);
		if (res != ERROR_SUCCESS) {
			printf("error\n");
		}

		std::string value_str((char*)lpbKeyValueData);
		if (value_str == expect_value) {
			//printf("found tap0901\n");
			dwKeyValueDataSize = 256;
			res = RegQueryValueExA(key, NetCfgName, NULL, &dwKeyValueType, lpbKeyValueData, &dwKeyValueDataSize);
			if (res != ERROR_SUCCESS) {
				printf("error\n");
				getchar();
			}
			std::string netcfg_str((char*)lpbKeyValueData);
			printf("Get netcfg: %s\n", netcfg_str.c_str());
			RegCloseKey(key);
			delete[] lpbKeyValueData;
			return netcfg_str;
		}


		RegCloseKey(key);
		delete[] lpbKeyValueData;

	}


	return "";
}


std::string TuntapWindows::GetDeviceName(std::string tun_guid) {
	if (tun_guid.empty()) return "";

	const auto connectionKey = TEXT(
		"SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}");

	std::string connectionKeyFull =
		std::string(connectionKey) + std::string(TEXT("\\")) + std::string(tun_guid.begin(), tun_guid.end()) +
		std::string(TEXT("\\Connection"));
	HKEY hkey;

	if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, connectionKeyFull.c_str(), 0, KEY_READ, &hkey)) {
		exit(-1);
	}

	DWORD dwKeyValueType;
	DWORD dwKeyValueDataSize = 256;

	LPBYTE lpbKeyValueData = new BYTE[256];
	const auto valueName = TEXT("Name");
	int res = RegQueryValueExA(hkey, valueName, NULL, &dwKeyValueType, lpbKeyValueData, &dwKeyValueDataSize);
	if (res != ERROR_SUCCESS) {
		printf("error\n");
	}

	return std::string((char*)lpbKeyValueData);
	
}







bool TuntapWindows::Open()
{

	auto tunGuid = TuntapWindows::GetTunDeviceID();
	auto tunAdapterName = TuntapWindows::GetDeviceName(tunGuid);
	auto tunDeviceFileName = std::string("\\\\.\\Global\\") + tunGuid + std::string(".tap");
	
	if (need_restart) goto restart;

	this->GetTunHandle() = CreateFile(tunDeviceFileName.c_str(), GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING,
		FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED, 0);

	if (this->GetTunHandle() == INVALID_HANDLE_VALUE) return false;

	DWORD len;

	struct in_addr ep[3];

	ep[0].S_un.S_un_b.s_b1 = 10;
	ep[0].S_un.S_un_b.s_b2 = 2;
	ep[0].S_un.S_un_b.s_b3 = 0;
	ep[0].S_un.S_un_b.s_b4 = 2;

	ep[1].S_un.S_un_b.s_b1 = 10;
	ep[1].S_un.S_un_b.s_b2 = 2;
	ep[1].S_un.S_un_b.s_b3 = 0;
	ep[1].S_un.S_un_b.s_b4 = 10;

	ep[2].S_un.S_un_b.s_b1 = 255;
	ep[2].S_un.S_un_b.s_b2 = 255;
	ep[2].S_un.S_un_b.s_b3 = 255;
	ep[2].S_un.S_un_b.s_b4 = 254;

	if (!DeviceIoControl(this->GetTunHandle(), TAP_WIN_IOCTL_CONFIG_TUN, ep, sizeof(ep), ep, sizeof(ep), &len, NULL)) {
		printf("WARNING: The TAP - Windows driver rejected a \
			TAP_WIN_IOCTL_CONFIG_TUN DeviceIoControl call");
		return false;
	}


	/*ULONG driverVersion[3] = { 0, 0, 0 };
	if (!DeviceIoControl(this->GetTunHandle(), TAP_WIN_IOCTL_GET_VERSION,
		&driverVersion, sizeof(driverVersion),
		&driverVersion, sizeof(driverVersion), &len, NULL)) {
		printf("WARNING: The TAP - Windows driver rejected a \
			TAP_WIN_IOCTL_GET_VERSION DeviceIoControl call");
		return false;
	}
	printf("TAP-Windows driver version %d.%d available.", driverVersion[0], driverVersion[1]);
	*/

restart:

	char status[4] = { 0x01, 0x00, 0x00, 0x00 };
	if (!DeviceIoControl(this->GetTunHandle(), TAP_WIN_IOCTL_SET_MEDIA_STATUS, status, 4, status, 4, &len, NULL)) {
		printf("WARNING: The TAP - Windows driver rejected a \
			TAP_WIN_IOCTL_SET_MEDIA_STATUS DeviceIoControl call");
		return false;
	}


	Sleep(2000);

	auto set_ip_script = std::string("netsh interface ipv4 set address name = ") + R"(")" + std::string(tunAdapterName.begin(), tunAdapterName.end()) + R"(")" +
		std::string(" source = static addr=10.2.0.2 mask=255.255.255.0 gateway=10.2.0.10 gwmetric=1");

	system(set_ip_script.c_str());

	return true;
}

bool TuntapWindows::Close()
{
	DWORD len;

	char status[4] = { 0x00, 0x00, 0x00, 0x00 };
	if (!DeviceIoControl(this->GetTunHandle(), TAP_WIN_IOCTL_SET_MEDIA_STATUS, status, 4, status, 4, &len, NULL)) {
		printf("WARNING: The TAP - Windows driver rejected a \
			TAP_WIN_IOCTL_SET_MEDIA_STATUS DeviceIoControl call");
		return false;
	}
	
	need_restart = true;
	return true;

}
#endif