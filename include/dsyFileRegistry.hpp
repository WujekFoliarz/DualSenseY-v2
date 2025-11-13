#pragma once

#ifdef WINDOWS
#include <windows.h>
#include <shlwapi.h>
#include <string>
#include <ShlObj_core.h>
inline static void RegisterFileAssociation() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring command = L"\"" + std::wstring(exePath) + L"\" \"%1\"";

    HKEY hKey;
    RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\.dsy", 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr);
    RegSetValueExW(hKey, nullptr, 0, REG_SZ, (BYTE*)L"DualSenseYFile", sizeof(L"DualSenseYFile"));
    RegCloseKey(hKey);

    RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\DualSenseYFile\\shell\\open\\command", 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr);
    RegSetValueExW(hKey, nullptr, 0, REG_SZ, (BYTE*)command.c_str(), (DWORD)((command.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(hKey);

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
}
#endif