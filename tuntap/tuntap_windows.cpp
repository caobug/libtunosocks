#include "tuntap_windows.h"
#include "tap-windows.h"
#include <string>

std::wstring TuntapWindows::GetTunDeviceID() 
{
	HKEY hkey;
	LPCWSTR adapterKey = TEXT("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}");

	if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, adapterKey, 0, KEY_READ, &hkey)) {
		exit(-1);
	}

	DWORD subKeyNum;
	DWORD subKeyNameMaxLen;
	RegQueryInfoKey(hkey, NULL, NULL, 0, &subKeyNum, &subKeyNameMaxLen, NULL, NULL, NULL, NULL, NULL, NULL);

	LPWSTR subKeyName = new WCHAR[subKeyNameMaxLen + 1];

	for (DWORD i = 1; i < subKeyNum; i++) {
		DWORD NameNum = subKeyNameMaxLen + 1;
		int res = RegEnumKeyEx(hkey, i, subKeyName, &NameNum, NULL, NULL, NULL, NULL);

		if (res != ERROR_SUCCESS) {
			break;
		}
		printf("%S\n", subKeyName);
		// open subkey 0000, 0001, 0002 ....

		DWORD dwKeyValueType;
		DWORD dwKeyValueDataSize = 256;

		LPBYTE lpbKeyValueData = new BYTE[256];

		HKEY key;
		std::wstring subkey = std::wstring(adapterKey) + std::wstring(TEXT("\\")) + std::wstring(subKeyName);
		res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, subkey.c_str(), 0, KEY_READ, &key);
		if (ERROR_SUCCESS != res) {
			continue;
		}


		const LPCWSTR valueName = TEXT("MatchingDeviceId");
		const LPCWSTR NetCfgName = TEXT("NetCfgInstanceId");
		const std::wstring expect_value(TEXT("tap0901"));
		res = RegQueryValueEx(key, valueName, NULL, &dwKeyValueType, lpbKeyValueData, &dwKeyValueDataSize);
		if (res != ERROR_SUCCESS) {
			printf("error\n");
		}

		std::wstring value_str((LPWSTR)lpbKeyValueData);
		if (value_str == expect_value) {
			printf("found tap0901\n");
			dwKeyValueDataSize = 256;
			res = RegQueryValueEx(key, NetCfgName, NULL, &dwKeyValueType, lpbKeyValueData, &dwKeyValueDataSize);
			if (res != ERROR_SUCCESS) {
				printf("error\n");
				getchar();
			}
			std::wstring netcfg_str((LPWSTR)lpbKeyValueData);
			printf("Get netcfg: %ls\n", netcfg_str.c_str());
			RegCloseKey(key);
			delete[] lpbKeyValueData;
			return netcfg_str;
		}


		RegCloseKey(key);
		delete[] lpbKeyValueData;

	}


	return std::wstring{L""};
}


std::wstring TuntapWindows::GetDeviceName(std::wstring tun_guid) {
	if (tun_guid.empty()) return std::wstring { L"" };

	const LPCWSTR connectionKey = TEXT(
		"SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}");

	std::wstring connectionKeyFull =
		std::wstring(connectionKey) + std::wstring(TEXT("\\")) + std::wstring(tun_guid.begin(), tun_guid.end()) +
		std::wstring(TEXT("\\Connection"));
	HKEY hkey;

	if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, connectionKeyFull.c_str(), 0, KEY_READ, &hkey)) {
		exit(-1);
	}

	DWORD dwKeyValueType;
	DWORD dwKeyValueDataSize = 256;

	LPBYTE lpbKeyValueData = new BYTE[256];
	const LPCWSTR valueName = TEXT("Name");
	int res = RegQueryValueEx(hkey, valueName, NULL, &dwKeyValueType, lpbKeyValueData, &dwKeyValueDataSize);
	if (res != ERROR_SUCCESS) {
		printf("error\n");
	}

	return std::wstring((LPWSTR)lpbKeyValueData);
	
}







bool TuntapWindows::Open()
{

	auto tunGuid = TuntapWindows::GetTunDeviceID();
	auto tunAdapterName = TuntapWindows::GetDeviceName(tunGuid);
	auto tunDeviceFileName = std::wstring(L"\\\\.\\Global\\") + tunGuid + std::wstring(L".tap");
	
	if (need_restart) goto restart;

	this->GetTunHandle() = CreateFile(tunDeviceFileName.c_str(), GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING,
		FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED, 0);

	if (this->GetTunHandle() == INVALID_HANDLE_VALUE) return false;

	DWORD len;

	struct in_addr ep[3];

	ep[0].S_un.S_un_b.s_b1 = 169;
	ep[0].S_un.S_un_b.s_b2 = 254;
	ep[0].S_un.S_un_b.s_b3 = 128;
	ep[0].S_un.S_un_b.s_b4 = 127;

	ep[1].S_un.S_un_b.s_b1 = 169;
	ep[1].S_un.S_un_b.s_b2 = 254;
	ep[1].S_un.S_un_b.s_b3 = 128;
	ep[1].S_un.S_un_b.s_b4 = 126;

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


	Sleep(1000);

	auto set_ip_script = std::string("netsh interface ipv4 set address name=") + R"(")" + std::string(tunAdapterName.begin(), tunAdapterName.end()) + R"(")" +
		std::string(" address=169.254.128.127 mask=255.255.255.0 gateway=169.254.128.126");
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