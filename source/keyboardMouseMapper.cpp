#include "keyboardMouseMapper.hpp"

#ifdef WINDOWS
#include <Windows.h>
#endif 

#include <chrono>
#include "scePadHandle.hpp"
#include <duaLib.h>


#ifdef WINDOWS
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

void MouseClick(DWORD flag, DWORD mouseData = 0) {
	INPUT input;
	input.type = INPUT_MOUSE;
	input.mi.mouseData = mouseData;
	input.mi.dwFlags = flag;
	input.mi.time = 0;
	input.mi.dwExtraInfo = 0;
	SendInput(1, &input, sizeof(INPUT));
}
#endif


void KeyboardMouseMapper::Thread() {
#ifdef WINDOWS
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	EXECUTION_STATE prevState = SetThreadExecutionState(
		ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED
	);

	HANDLE hTimer = CreateWaitableTimerEx(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
	LARGE_INTEGER liDueTime;
	liDueTime.QuadPart = -5000LL;

	std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();

	while (m_ThreadRunning) {

		static bool wHeld = false, aHeld = false, sHeld = false, dHeld = false;
		static std::chrono::milliseconds time = std::chrono::milliseconds(100);
		const int DEADZONE = 20;

		bool fire = false;
		if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - lastTime) > time) {
			fire = true;
		}

		for (int i = 0; i < 4; i++) {
			s_ScePadData state = {};
			int result = scePadReadState(g_ScePad[i], &state);

			if (result != SCE_OK || m_ScePadSettings == nullptr)
				continue;

		#pragma region Touchpad as mouse
			if (m_ScePadSettings[i].touchpadAsMouse && !state.touchData.touch[0].reserve[0]) {

				if (!m_ScePadSettings[i].wasTouching) {
					m_ScePadSettings[i].lastTouchData.touch[0].x = state.touchData.touch[0].x;
					m_ScePadSettings[i].lastTouchData.touch[0].y = state.touchData.touch[0].y;
				}

				int cursorX = state.touchData.touch[0].x - m_ScePadSettings[i].lastTouchData.touch[0].x;
				int cursorY = state.touchData.touch[0].y - m_ScePadSettings[i].lastTouchData.touch[0].y;

				float sensitivity = m_ScePadSettings[i].touchpadAsMouse_sensitivity;
				if (state.touchData.touch[1].reserve[0] && (abs(cursorX) > 3 || abs(cursorY) > 3))
					MoveCursor((float)cursorX * sensitivity, (float)cursorY * sensitivity);

				m_ScePadSettings[i].lastTouchData.touch[0].reserve[0] = state.touchData.touch[0].reserve[0];
				m_ScePadSettings[i].lastTouchData.touch[0].x = state.touchData.touch[0].x;
				m_ScePadSettings[i].lastTouchData.touch[0].y = state.touchData.touch[0].y;
				m_ScePadSettings[i].wasTouching = true;

				if (!state.touchData.touch[1].reserve[0] && fabs(cursorY) > 5.0f) {
					MouseClick(MOUSEEVENTF_WHEEL, static_cast<int>(-cursorY * 2));
				}
			}
			else if (m_ScePadSettings[i].touchpadAsMouse && state.touchData.touch[0].reserve[0]) {
				m_ScePadSettings[i].wasTouching = false;
			}

			static bool wasLeftMousePressed = false;
			static bool wasRightMousePressed = false;
			if (m_ScePadSettings[i].touchpadAsMouse && state.touchData.touch[0].x < 1000 && state.bitmask_buttons & SCE_BM_TOUCH) {
				MouseClick(MOUSEEVENTF_LEFTDOWN);
				wasLeftMousePressed = true;
			}
			else if (m_ScePadSettings[i].touchpadAsMouse && state.touchData.touch[0].x > 1000 && state.bitmask_buttons & SCE_BM_TOUCH) {
				wasRightMousePressed = true;
				MouseClick(MOUSEEVENTF_RIGHTDOWN);
			}

			if (m_ScePadSettings[i].touchpadAsMouse && wasLeftMousePressed && !(state.bitmask_buttons & SCE_BM_TOUCH)) {
				MouseClick(MOUSEEVENTF_LEFTUP);
				wasLeftMousePressed = false;
			}
			if (m_ScePadSettings[i].touchpadAsMouse && wasRightMousePressed && !(state.bitmask_buttons & SCE_BM_TOUCH)) {
				MouseClick(MOUSEEVENTF_RIGHTUP);
				wasRightMousePressed = false;
			}
		#pragma endregion

		#pragma region Emulate analog wsad
			if (m_ScePadSettings[i].emulateAnalogWsad) {
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
		#pragma endregion

		#pragma region Gyro to mouse

			if (m_ScePadSettings[i].gyroToMouse) {
				static bool lastVelX[4] = { 0 };
				static bool lastVelY[4] = { 0 };

				float velX = -state.angularVelocity.z;
				float velY = -state.angularVelocity.x;

				float X = ((velX - lastVelX[i]) / 100.0f) * m_ScePadSettings[i].gyroToMouseSensitivity;
				float Y = ((velY - lastVelY[i]) / 100.0f) * m_ScePadSettings[i].gyroToMouseSensitivity;

				if(abs(X) > 0.1f && abs(Y) > 0.1f)
					MoveCursor(X, Y);

				lastVelX[i] = velX;
				lastVelY[i] = velY;
			}

		#pragma endregion

		#pragma region Mouse1 hotkey
			if (m_ScePadSettings[i].useMouse1Hotkey) {
				static bool wasPressed[4] = { false };

				if ((state.bitmask_buttons & m_ScePadSettings[i].mouse1Hotkey) && !wasPressed[i]) {
					MouseClick(MOUSEEVENTF_LEFTDOWN);
					wasPressed[i] = true;
				}
				else if (!(state.bitmask_buttons & m_ScePadSettings[i].mouse1Hotkey) && wasPressed[i]) {
					MouseClick(MOUSEEVENTF_LEFTUP);
					wasPressed[i] = false;
				}
			}

		#pragma endregion
		}

		if (fire) {
			lastTime = std::chrono::steady_clock::now();
		}

		SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0);
		WaitForSingleObject(hTimer, INFINITE);
	}
#endif
}

void KeyboardMouseMapper::MoveCursor(int x, int y) {
#ifdef WINDOWS
	INPUT input = { 0 };
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_MOVE;
	input.mi.dx = x;
	input.mi.dy = y;

	SendInput(1, &input, sizeof(INPUT));
#endif
}

KeyboardMouseMapper::KeyboardMouseMapper(s_scePadSettings* scePadSettings) : m_ScePadSettings(scePadSettings) {
#ifdef WINDOWS
	m_thread = std::thread(&KeyboardMouseMapper::Thread, this);
	m_thread.detach();
#endif
}

KeyboardMouseMapper::~KeyboardMouseMapper() {
#ifdef WINDOWS
	m_ThreadRunning = false;

	if (m_thread.joinable()) {
		m_thread.join();
	}
#endif
}
