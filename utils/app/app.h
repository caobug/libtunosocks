#pragma once
#include "readystate_checker.h"
#include "singleinstanceapp.h"
#include "../logger.h"

class App
{
public:

    static void Init()
    {
        Logger::GetInstance()->InitLog();
        Logger::GetInstance()->SetLevel(spdlog::level::debug);

        SingleInstanceApp::GetInstance()->Init();

        ReadStateChecker::GetInstance()->Run();
    }

};