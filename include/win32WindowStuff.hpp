#ifndef WINDOWTRANSPARENCY_H
#define WINDOWTRANSPARENCY_H

#if defined(_WIN32) && (!defined(__linux__) && !defined(__APPLE__))
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

void SetDarkTitleBar(HWND hwnd, bool enabled) {
    BOOL useDark = enabled ? TRUE : FALSE;
    // 20 = DWMWA_USE_IMMERSIVE_DARK_MODE on Windows 10 1809+
    DwmSetWindowAttribute(hwnd, 20, &useDark, sizeof(useDark));
}

struct ACCENT_POLICY {
    int nAccentState;
    int nFlags;
    int nColor;
    int nAnimationId;
};

struct WINCOMPATTRDATA {
    int nAttribute;
    PVOID pData;
    ULONG ulDataSize;
};

enum AccentState {
    ACCENT_DISABLED = 0,
    ACCENT_ENABLE_GRADIENT = 1,
    ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
    ACCENT_ENABLE_BLURBEHIND = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4, // Windows 10 1803+
    ACCENT_ENABLE_HOSTBACKDROP = 5,      // Windows 11 Mica
};

void EnableBlurBehind(HWND hwnd) {
    ACCENT_POLICY policy = {};
    policy.nAccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
    policy.nFlags = 2; 
    policy.nColor = 0x990000; 

    WINCOMPATTRDATA data;
    data.nAttribute = 19; 
    data.pData = &policy;
    data.ulDataSize = sizeof(policy);

    auto setWindowCompositionAttribute =
        (BOOL(WINAPI*)(HWND, WINCOMPATTRDATA*))GetProcAddress(
            GetModuleHandleA("user32.dll"), "SetWindowCompositionAttribute");

    if (setWindowCompositionAttribute)
        setWindowCompositionAttribute(hwnd, &data);
}
#endif

#endif // WINDOWTRANSPARENCY_H