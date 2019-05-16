#pragma once

#if defined(BUILD_DLL) && defined(_WIN32)
#define OS_Dll_API   __declspec( dllexport )
#else
#define OS_Dll_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

OS_Dll_API int tunosocks_install_driver();
OS_Dll_API int tunosocks_uninstall_driver();

OS_Dll_API void tunosocks_setsocks5(const char* ip, unsigned short port);

OS_Dll_API int  tunosocks_start();

OS_Dll_API void tunosocks_stop();

#ifdef __cplusplus
}
#endif

