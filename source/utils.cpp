#include "utils.hpp"
#include <algorithm>
#include "log.hpp"
#ifdef WINDOWS
#include <Windows.h>
#endif

// Function to retrieve HidHide installation path from Windows registry
std::string getHidHideExecutablePath() {
#ifdef WINDOWS
    HKEY hKey = NULL;
    std::string hidHidePath = "";
    
    // Try to open the registry key for HidHide from Nefarius Software Solutions e.U.
    LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Nefarius Software Solutions e.U.\\HidHide", 0, KEY_READ, &hKey);
    
    // If not found, try the WOW6432Node (32-bit registry on 64-bit systems)
    if (result != ERROR_SUCCESS) {
        result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Nefarius Software Solutions e.U.\\HidHide", 0, KEY_READ, &hKey);
    }
    
    if (result == ERROR_SUCCESS) {
        DWORD dataSize = MAX_PATH;
        char installPath[MAX_PATH] = {0};
        
        // Try to read the Path value
        result = RegQueryValueExA(hKey, "Path", NULL, NULL, (LPBYTE)installPath, &dataSize);
        
        if (result == ERROR_SUCCESS && dataSize > 0) {
            hidHidePath = std::string(installPath);
            // Ensure proper path formatting and append the x64 executable path
            if (hidHidePath.back() != '\\') {
                hidHidePath += "\\";
            }
            hidHidePath += "x64\\HidHideCLI.exe";
            LOGI("Found HidHide at: " + hidHidePath);
        }
        
        RegCloseKey(hKey);
    } else {
        // Fallback: Try common installation paths from Nefarius Software Solutions
        std::vector<std::string> commonPaths = {
            "C:\\Program Files\\Nefarius Software Solutions\\HidHide\\x64\\HidHideCLI.exe",
            "C:\\Program Files\\Nefarius Software Solutions\\HidHide\\x86\\HidHideCLI.exe",
            "C:\\Program Files (x86)\\Nefarius Software Solutions\\HidHide\\x86\\HidHideCLI.exe"
        };
        
        for (const auto& path : commonPaths) {
            if (std::filesystem::exists(path)) {
                hidHidePath = path;
                LOGI("Found HidHide at common path: " + hidHidePath);
                break;
            }
        }
    }
    
    // If not found, show alert message
    if (hidHidePath.empty()) {
        std::string errorMsg = "HidHide is not installed on your system.\n\n"
                               "Please install HidHide from: https://github.com/nefarius/HidHide\n\n"
                               "Without HidHide, the controller hiding feature will not work.";
        LOGE(errorMsg);
        MessageBoxA(NULL, errorMsg.c_str(), "HidHide Not Found", MB_OK | MB_ICONWARNING);
    }
    
    return hidHidePath;
#else
    return "";
#endif
}

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

    // Get HidHide executable path from registry or common installation paths
    std::string hidHideExePath = getHidHideExecutablePath();
    
    if (hidHideExePath.empty()) {
        LOGE("HidHide executable not found. Controller hiding feature is disabled.");
        return;
    }

    std::string command = hidHideExePath + " " + arg1 + arg2 + arg3;

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
        LOGE("Failed to start HidHide.exe from path: " + command);
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

void HideController(const std::string& instanceId) {
#if !defined(__linux__) && !defined(__MACOS__)
    hidHideRequest(instanceId, "hide");
#endif
}

void UnhideController(const std::string& instanceId) {
#if !defined(__linux__) && !defined(__MACOS__)
    hidHideRequest(instanceId, "show");
#endif
}

bool IsRunningAsAdministratorWindows() {
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