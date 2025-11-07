#include "appMutex.hpp"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <platform_folders.h>

#ifdef WINDOWS
#include <Windows.h>
#else
#include <unistd.h>
#endif

static inline std::filesystem::path directory = std::filesystem::path(sago::getDocumentsFolder() + "/DSY/");

bool IsAlreadyRunning(const std::string& LockName) {
    namespace fs = std::filesystem;

    fs::path path = directory / LockName;

    if (fs::exists(path)) {
        std::ifstream pidFile(path);
        int pid;
        if (pidFile >> pid) {
        #if defined(_WIN32)
            HANDLE h = OpenProcess(SYNCHRONIZE, FALSE, pid);
            if (h != NULL) {
                CloseHandle(h);
                return true; 
            }
        #else
            if (kill(pid, 0) == 0) {
                return true;
            }
        #endif
        }

    }

    std::ofstream pidFile(path, std::ios::trunc);
#ifdef WINDOWS
    pidFile << GetCurrentProcessId();
#else
    pidFile << getpid();
#endif
    pidFile.close();
    return false;
}
