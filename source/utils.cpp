#include "utils.hpp"
#include <algorithm>
#include "log.hpp"
#ifdef WINDOWS
#include <Windows.h>
#endif

void hidHideRequest(std::string ID, std::string arg) {
#ifdef WINDOWS
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW; // Use this flag to control window visibility
    si.wShowWindow = SW_HIDE;          // Set to SW_HIDE to prevent the window from showing

    ZeroMemory(&pi, sizeof(pi));

    std::string arg1 = "\"" + ID + "\"";
    std::string arg2 = " \"" + arg + "\" ";

    std::filesystem::path path = std::filesystem::current_path();
    std::string exePath = "";

    if (std::filesystem::exists(path.string() + "\\DSX.exe")) {
        exePath = path.string() + "\\DSX.exe";
    }
    else {
        exePath = path.string() + "\\DualSenseY.exe";
    }

    std::string arg3 = " \"" + exePath + "\" ";

    std::string command = RESOURCES_PATH "/externals/windows/HidHide.exe " + arg1 + arg2 + arg3;

    if (CreateProcess(NULL,              // No module name (use command line)
        (LPSTR)command.c_str(), // Command line
        NULL,               // Process handle not inheritable
        NULL,               // Thread handle not inheritable
        FALSE,              // Set handle inheritance to FALSE
        0,                  // No creation flags
        NULL,               // Use parent's environment block
        NULL,               // Use parent's starting directory 
        &si,                // Pointer to STARTUPINFO structure
        &pi)                // Pointer to PROCESS_INFORMATION structure
       ) {
        // Close process and thread handles. 
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else {
        LOGE("Failed to start HidHide.exe");
    }
#endif
}

std::string USBtoHIDinstance(const std::string& input) {
    std::string result = input;

    // Replace "USB" with "HID"
    size_t pos = result.find("USB");
    if (pos != std::string::npos) {
        result.replace(pos, 3, "HID");
    }

    // Convert the string to uppercase
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);

    return result;
}

void hideController(const std::string& instanceId) {
#if !defined(__linux__) && !defined(__MACOS__)
    hidHideRequest(instanceId, "hide");
#endif
}

void unhideController(const std::string& instanceId) {
#if !defined(__linux__) && !defined(__MACOS__)
    hidHideRequest(instanceId, "show");
#endif
}

bool isRunningAsAdministratorWindows() {
#if !defined(__linux__) && !defined(__MACOS__)
    BOOL fIsRunAsAdmin = FALSE;
    DWORD dwError = ERROR_SUCCESS;
    PSID pAdministratorsGroup = NULL;

    // Allocate and initialize a SID of the administrators group.
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (!AllocateAndInitializeSid(
        &NtAuthority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &pAdministratorsGroup)) {
        dwError = GetLastError();
        goto Cleanup;
    }

    // Determine whether the SID of administrators group is enabled in 
    // the primary access token of the process.
    if (!CheckTokenMembership(NULL, pAdministratorsGroup, &fIsRunAsAdmin)) {
        dwError = GetLastError();
        goto Cleanup;
    }

Cleanup:
    // Centralized cleanup for all allocated resources.
    if (pAdministratorsGroup) {
        FreeSid(pAdministratorsGroup);
        pAdministratorsGroup = NULL;
    }

    // Throw the error if something failed in the function.
    if (ERROR_SUCCESS != dwError) {
        throw dwError;
    }

    return fIsRunAsAdmin;
#endif
    return false;
}

void DisableBluetoothDevice(const std::string& Address) {
    if (Address == "")
        return;

#ifdef WINDOWS
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    std::string cleanMac = Address;
    cleanMac.erase(std::remove(cleanMac.begin(), cleanMac.end(), ':'), cleanMac.end());

    std::string command = RESOURCES_PATH "externals/windows/BTControl.exe " + cleanMac;
    LOGI("BTControl command: %s", command.c_str());

    if (CreateProcess(NULL,
        (LPSTR)command.c_str(),
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi)
    ) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
#endif
}