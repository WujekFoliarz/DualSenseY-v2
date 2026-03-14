#include "utils.hpp"
#include <algorithm>
#include "log.hpp"
#ifdef WINDOWS
#include <Windows.h>
#include <cfgmgr32.h>
#include <vector>
#include <string>
#endif

#ifdef WINDOWS
static std::wstring Utf8ToWstring(const std::string& str)
{
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), NULL, 0);
    if (size_needed <= 0) return std::wstring();
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), &wstr[0], size_needed);
    return wstr;
}

std::string GetDeviceInstancePath(const std::string& lastPath) {
    // Remove prefix "\\?\" 
    std::string path = lastPath;

    if (path.compare(0, 4, "\\\\?\\") == 0) {
        path.erase(0, 4);
    }

	// Look for last GUID (starts with '{' ) because BT devices have in-between GUID
    size_t lastGuidPos = path.rfind("#{");
    if (lastGuidPos != std::string::npos) {
        path.erase(lastGuidPos);
    }

    // Replace '#' with '\' for HID format
    std::replace(path.begin(), path.end(), '#', '\\');

    return path;
}
#endif

bool ReplugDevice(const std::wstring& instanceId)
{
#ifdef WINDOWS
    DEVINST devInst;
    CONFIGRET status = CM_Locate_DevNodeW(
        &devInst,
        const_cast<LPWSTR>(instanceId.c_str()),
        CM_LOCATE_DEVNODE_NORMAL
    );

    if (status != CR_SUCCESS)
    {
        std::wcout << L"Device not found:\n";
        std::wcout << instanceId.c_str() << L"\n";
        return false;
    }

    CM_Disable_DevNode(devInst, 0);
    Sleep(2000);
    CM_Enable_DevNode(devInst, 0);
#endif

    return true;
}


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
            LOGI("Found HidHide at: %s", hidHidePath.c_str());
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
                LOGI("Found HidHide at common path: %s", hidHidePath.c_str());
                break;
            }
        }
    }
    
    // If not found, show alert message
    if (hidHidePath.empty()) {
        std::string errorMsg = "HidHide is not installed on your system.\n\n"
                               "Please install HidHide from: https://github.com/nefarius/HidHide\n\n"
                               "Without HidHide, the controller hiding feature will not work.";
        LOGE("%s", errorMsg.c_str());
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
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    ZeroMemory(&pi, sizeof(pi));

    // Get HidHide executable path
    std::string hidHideExePath = getHidHideExecutablePath();
    
    if (hidHideExePath.empty()) {
        LOGE("HidHide executable not found. Controller hiding feature is disabled.");
        return;
    }

    // Build command based on the requested action
    std::string command = "\"" + hidHideExePath + "\"";

    std::string hidDeviceInstancePath = GetDeviceInstancePath(ID);
    
    if (arg == "hide") {
        command += " --cloak-on --dev-hide \"" + hidDeviceInstancePath + "\"";
        LOGI("Executing HidHide: hide device %s", hidDeviceInstancePath);
    }
    else if (arg == "show") {
        command += " --cloak-off --dev-unhide \"" + hidDeviceInstancePath + "\"";
        LOGI("Executing HidHide: unhide device %s", hidDeviceInstancePath);
    }
    else {
        LOGE("Invalid argument for hidHideRequest. Only 'hide' and 'show' are supported.");
        return;
    }

    // Execute the HidHide CLI command
    if (CreateProcess(NULL,
        (LPSTR)command.c_str(),
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi)) {
        // Wait for the process to complete
        WaitForSingleObject(pi.hProcess, INFINITE);
        
        // Get exit code to verify success
        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        
        if (exitCode == 0) {
            LOGI("HidHide command completed successfully");
            ReplugDevice(Utf8ToWstring(hidDeviceInstancePath));
        } else {
            LOGE("HidHide command failed with exit code: %lu", static_cast<unsigned long>(exitCode));
        }
        
        // Close process and thread handles
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else {
        LOGE("Failed to execute HidHide command. Error: %lu", static_cast<unsigned long>(GetLastError()));
    }
#endif
}

// Function to get the path of the current executable
std::string getCurrentExecutablePath() {
#ifdef WINDOWS
    std::vector<char> buffer(MAX_PATH);
    DWORD size;

    while (true) {
        size = GetModuleFileNameA(NULL, buffer.data(), static_cast<DWORD>(buffer.size()));
        
        if (size == 0) return ""; // Error
        
        // If buffer small, make it bigger and try again
        if (size == buffer.size()) {
            buffer.resize(buffer.size() * 2);
        } else {
            break; // Success
        }
    }
    return std::string(buffer.data(), size);
#else
    return "";
#endif
}

// Function to register this application with HidHide so it can always see hidden devices
void RegisterApplicationWithHidHide() {
#ifdef WINDOWS
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    ZeroMemory(&pi, sizeof(pi));

    std::string appPath = getCurrentExecutablePath();
    if (appPath.empty()) {
        LOGE("Failed to get current executable path for HidHide registration");
        return;
    }

    // Get HidHide executable path
    std::string hidHideExePath = getHidHideExecutablePath();
    if (hidHideExePath.empty()) {
        LOGE("HidHide executable not found. Cannot register application.");
        return;
    }

    // Build command to register this application: HidHideCLI.exe --app-reg "<app_path>"
    std::string command = "\"" + hidHideExePath + "\" --app-reg \"" + appPath + "\"";

    if (CreateProcess(NULL,
        (LPSTR)command.c_str(),
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi)) {
        // Wait for the process to complete
        WaitForSingleObject(pi.hProcess, INFINITE);
        
        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        
        if (exitCode == 0) {
            LOGI("Application successfully registered with HidHide: %s", appPath.c_str());
        } else {
            LOGE("Failed to register application with HidHide. Exit code: %lu", static_cast<unsigned long>(exitCode));
        }
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else {
        LOGE("Failed to execute HidHide registration command. Error: %lu", static_cast<unsigned long>(GetLastError()));
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
