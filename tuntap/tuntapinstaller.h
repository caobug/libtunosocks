#ifdef _WIN32
#pragma once

#include "../utils/singleton.h"
#include "../utils/filehelper.h"

#include <Windows.h>

//we don't support 32bit
class TuntapInstaller : public Singleton<TuntapInstaller>
{

	
public:
	TuntapInstaller()
	{
		current_path = FileHelper::GetCurrentDir();
		current_path = "C:\\";
	}

	bool Find()
	{
		if (!isRunningasAdmin()) return false;

		std::string tapinstaller_path = current_path + tapinstaller_path_prefix;

		std::string cmd = tapinstaller_path + std::string{ " find " } + std::string{ " tap0901" };

		if (startup(tapinstaller_path, cmd) != 0) return false;

		return true;
	}

	bool Install()
	{
		if (!isRunningasAdmin()) return false; 
			
		std::string tapinstaller_path = current_path + tapinstaller_path_prefix;
		std::string tapinstaller_inf_path = current_path + inf_path_prefix;

		std::string cmd = tapinstaller_path + std::string{" install "} + tapinstaller_inf_path + std::string{" tap0901"};

		if (startup(tapinstaller_path, cmd) != 0) return false;

		return true;
	}

	bool Uninstall()
	{
		if (!isRunningasAdmin()) return false;

		std::string tapinstaller_path = current_path + tapinstaller_path_prefix;
		std::string tapinstaller_inf_path = current_path + inf_path_prefix;

		std::string cmd = tapinstaller_path + std::string{ " remove " } +std::string{ " tap0901" };

		if (startup(tapinstaller_path, cmd) != 0) return false;

		return true;
	}

private:
	std::string current_path;
	const std::string tapinstaller_path_prefix{ "tap-driver\\devcon.exe" };
	const std::string inf_path_prefix{ "tap-driver\\OemVista.inf" };


	BOOL isRunningasAdmin()
	{
		BOOL bElevated = FALSE;
		HANDLE hToken = NULL;

		// Get current process token
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
			return FALSE;

		TOKEN_ELEVATION tokenEle;
		DWORD dwRetLen = 0;

		// Retrieve token elevation information
		if (GetTokenInformation(hToken, TokenElevation, &tokenEle, sizeof(tokenEle), &dwRetLen))
		{
			if (dwRetLen == sizeof(tokenEle))
			{
				bElevated = tokenEle.TokenIsElevated;
			}
		}

		CloseHandle(hToken);
		return bElevated;
	}



	DWORD startup(std::string app_path, std::string cmd)
	{
		//wprintf(L"app_path: %s", app_path.c_str());
		// additional information
		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		// set the size of the structures
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;

		ZeroMemory(&pi, sizeof(pi));

		// start the program up
		int res = CreateProcessA(app_path.c_str(),   // the path
			const_cast<char*>(cmd.c_str()),        // Command line
			NULL,           // Process handle not inheritable
			NULL,           // Thread handle not inheritable
			FALSE,          // Set handle inheritance to FALSE
			0,              // No creation flags
			NULL,           // Use parent's environment block
			NULL,           // Use parent's starting directory 
			&si,            // Pointer to STARTUPINFO structure
			&pi             // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
		);

		if (res == 0)
		{
			printf("CreateProcess err %d", GetLastError());
			return -1;
		}

		WaitForSingleObject(pi.hProcess, INFINITE);

		DWORD exitCode = 0;
		if (!GetExitCodeProcess(pi.hProcess, &exitCode))
		{
			printf("tapinstall get exit code err\n");
			return -1;
		}

		//printf("tapinstall exit code %d\n", exitCode);

		// Close process and thread handles. 
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		return exitCode;
	}


};
#endif
