#include "system_exec.h"
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <array>

#ifdef _WIN32
#define PCLOSE _pclose
#define POPEN _popen
#else
#define PCLOSE pclose
#define POPEN popen
#endif

std::string ExecAndGetRes(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&PCLOSE)> pipe(POPEN(cmd, "r"), PCLOSE);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}