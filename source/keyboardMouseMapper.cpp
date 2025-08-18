#include "keyboardMouseMapper.hpp"

#if !defined(__linux__) && !defined(__MACOS__)
#include <Windows.h>
#endif 

#include <chrono>
#include "scePadHandle.hpp"
#include <duaLib.h>


#if !defined(__linux__) && !defined(__MACOS__)
constexpr WORD SC_W = 0x11;
constexpr WORD SC_A = 0x1E;
constexpr WORD SC_S = 0x1F;
constexpr WORD SC_D = 0x20;

void sendKeyScan(WORD scancode, bool down) {
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.dwFlags = KEYEVENTF_SCANCODE | (down ? 0 : KEYEVENTF_KEYUP);
    input.ki.wScan = scancode;
    SendInput(1, &input, sizeof(INPUT));
}
#endif


void KeyboardMouseMapper::thread() {
#if !defined(__linux__) && !defined(__MACOS__)
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	HANDLE hTimer = CreateWaitableTimer(NULL, TRUE, NULL);
	LARGE_INTEGER liDueTime;
	liDueTime.QuadPart = -1000LL;

    std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();

	while (m_threadRunning) {

        static bool wHeld = false, aHeld = false, sHeld = false, dHeld = false;
        static std::chrono::milliseconds time = std::chrono::milliseconds(100);
        const int DEADZONE = 20;

        bool fire = false;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - lastTime) > time) {
            fire = true;
        }

        for (int i = 0; i < 4; i++) {
            s_ScePadData state = {};
            int result = scePadReadState(g_scePad[i], &state);

            if (result != SCE_OK || m_scePadSettings == nullptr)
                continue;

            if(m_scePadSettings[i].emulateAnalogWsad)
            {
                int lx = state.LeftStick.X;
                int ly = state.LeftStick.Y;

                // W = stick up
                bool wNow = (ly < 128 - DEADZONE);
                if (wNow != wHeld) {
                    sendKeyScan(SC_W, wNow);
                    wHeld = wNow;
                }

                // S = stick down
                bool sNow = (ly > 128 + DEADZONE);
                if (sNow != sHeld) {
                    sendKeyScan(SC_S, sNow);
                    sHeld = sNow;
                }

                // A = stick left
                bool aNow = (lx < 128 - DEADZONE);
                if (aNow != aHeld) {
                    sendKeyScan(SC_A, aNow);
                    aHeld = aNow;
                }

                // D = stick right
                bool dNow = (lx > 128 + DEADZONE);
                if (dNow != dHeld) {
                    sendKeyScan(SC_D, dNow);
                    dHeld = dNow;
                }

                if (wNow && dNow && fire) {
                    sendKeyScan(SC_D, dNow);
                }

                if (wNow && aNow && fire) {
                    sendKeyScan(SC_A, aNow);
                }
            }
        }

        if (fire) {
            lastTime = std::chrono::steady_clock::now();
        }

		SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0);
		WaitForSingleObject(hTimer, INFINITE);
	}
#endif
}

KeyboardMouseMapper::KeyboardMouseMapper(s_scePadSettings* scePadSettings) : m_scePadSettings(scePadSettings) {
#if !defined(__linux__) && !defined(__MACOS__)
	m_thread = std::thread(&KeyboardMouseMapper::thread, this);
	m_thread.detach();
#endif
}

KeyboardMouseMapper::~KeyboardMouseMapper() {
#if !defined(__linux__) && !defined(__MACOS__)
    m_threadRunning = false;

    if (m_thread.joinable()) {
        m_thread.join();
    }
#endif
}
