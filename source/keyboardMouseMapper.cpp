#include "keyboardMouseMapper.hpp"

#ifdef WINDOWS
#include <Windows.h>
#endif

#include <chrono>
#include "scePadHandle.hpp"
#include <duaLib.h>
#include <controllerHotkey.hpp>

#ifdef WINDOWS
constexpr WORD SC_W = 0x11;
constexpr WORD SC_A = 0x1E;
constexpr WORD SC_S = 0x1F;
constexpr WORD SC_D = 0x20;

void sendKeyScan(WORD scancode, bool down)
{
	INPUT input = {};
	input.type = INPUT_KEYBOARD;
	input.ki.dwFlags = KEYEVENTF_SCANCODE | (down ? 0 : KEYEVENTF_KEYUP);
	input.ki.wScan = scancode;

	SendInput(1, &input, sizeof(INPUT));
}

void sendVirtualKey(WORD vk, bool down)
{
	INPUT input = {};
	input.type = INPUT_KEYBOARD;
	input.ki.wVk = vk;
	input.ki.wScan = static_cast<WORD>(MapVirtualKey(vk, MAPVK_VK_TO_VSC));
	input.ki.dwFlags = (down ? 0 : KEYEVENTF_KEYUP) | KEYEVENTF_EXTENDEDKEY;
	SendInput(1, &input, sizeof(INPUT));
}

void MouseClick(DWORD flag, DWORD mouseData = 0)
{
	INPUT input;
	input.type = INPUT_MOUSE;
	input.mi.mouseData = mouseData;
	input.mi.dwFlags = flag;
	input.mi.time = 0;
	input.mi.dwExtraInfo = 0;
	SendInput(1, &input, sizeof(INPUT));
}
#endif

void KeyboardMouseMapper::Thread()
{
#ifdef WINDOWS
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	EXECUTION_STATE prevState = SetThreadExecutionState(
		ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED);

	HANDLE hTimer = CreateWaitableTimerEx(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
	LARGE_INTEGER liDueTime;
	liDueTime.QuadPart = -5000LL;

	std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();

	while (m_ThreadRunning)
	{

		static bool wHeld = false, aHeld = false, sHeld = false, dHeld = false;
		static std::chrono::milliseconds time = std::chrono::milliseconds(100);
		const int DEADZONE = 20;

		bool fire = false;
		if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - lastTime) > time)
		{
			fire = true;
		}

		for (int i = 0; i < 4; i++)
		{
			s_ScePadData state = {};
			int result = scePadReadState(g_ScePad[i], &state);

			if (result != SCE_OK || m_ScePadSettings == nullptr)
			{
				if (m_psState[i].isDown)
				{
					sendVirtualKey(VK_LWIN, false);
					m_psState[i].isDown = false;
					m_psState[i].releasePending = false;
				}
				continue;
			}

#pragma region Touchpad as mouse
			if (m_ScePadSettings[i].touchpadAsMouse)
			{
				auto& ts = m_touchState[i];
				bool finger0Down = !state.touchData.touch[0].reserve[0];
				bool finger1Down = !state.touchData.touch[1].reserve[0];

				if (finger0Down)
				{
					float curX = static_cast<float>(state.touchData.touch[0].x);
					float curY = static_cast<float>(state.touchData.touch[0].y);

					if (!ts.wasTouching)
					{
						ts.lastTouchX = curX;
						ts.lastTouchY = curY;
						ts.smoothedDeltaX = 0.0f;
						ts.smoothedDeltaY = 0.0f;
						ts.accumX = 0.0f;
						ts.accumY = 0.0f;
						ts.accumScrollY = 0.0f;
						ts.wasTouching = true;
					}

					float rawDeltaX = curX - ts.lastTouchX;
					float rawDeltaY = curY - ts.lastTouchY;
					ts.lastTouchX = curX;
					ts.lastTouchY = curY;

					if (!finger1Down)
					{
						// Single finger mode: Cursor movement
						float magSq = rawDeltaX * rawDeltaX + rawDeltaY * rawDeltaY;
						// Gentle noise gate (0.25f units squared -> delta 0.5)
						if (magSq > 0.25f)
						{
							float speed = std::sqrt(magSq);
							// Dynamic EMA alpha: higher responsiveness on fast swipes, smoother filtering on slow micro-movements
							float alpha = (speed > 10.0f) ? 0.75f : 0.45f;
							ts.smoothedDeltaX = alpha * rawDeltaX + (1.0f - alpha) * ts.smoothedDeltaX;
							ts.smoothedDeltaY = alpha * rawDeltaY + (1.0f - alpha) * ts.smoothedDeltaY;

							// Pointer ballistics: mild acceleration curve
							float sensitivity = m_ScePadSettings[i].touchpadAsMouse_sensitivity;
							float extraSpeed = speed / 16.0f;
							if (extraSpeed > 1.6f) extraSpeed = 1.6f;
							float speedFactor = 1.0f + extraSpeed;
							float moveDeltaX = ts.smoothedDeltaX * sensitivity * 0.75f * speedFactor;
							float moveDeltaY = ts.smoothedDeltaY * sensitivity * 0.75f * speedFactor;

							// Sub-pixel accumulator
							ts.accumX += moveDeltaX;
							ts.accumY += moveDeltaY;

							int stepX = static_cast<int>(ts.accumX);
							int stepY = static_cast<int>(ts.accumY);

							if (stepX != 0 || stepY != 0)
							{
								MoveCursor(stepX, stepY);
								ts.accumX -= stepX;
								ts.accumY -= stepY;
							}
						}
						else
						{
							ts.smoothedDeltaX *= 0.5f;
							ts.smoothedDeltaY *= 0.5f;
						}
					}
					else
					{
						// Two finger mode: Smooth scroll
						ts.accumScrollY += -rawDeltaY * 3.5f;
						int scrollTicks = static_cast<int>(ts.accumScrollY / 10.0f);
						if (scrollTicks != 0)
						{
							MouseClick(MOUSEEVENTF_WHEEL, scrollTicks * WHEEL_DELTA / 4);
							ts.accumScrollY -= scrollTicks * 10.0f;
						}
					}
				}
				else
				{
					// Finger released
					ts.wasTouching = false;
					ts.smoothedDeltaX = 0.0f;
					ts.smoothedDeltaY = 0.0f;
					ts.accumX = 0.0f;
					ts.accumY = 0.0f;
					ts.accumScrollY = 0.0f;
				}

				// Touchpad physical click logic:
				// Default Click = LPM, Hold (> 380ms) = PPM, 2-finger click = PPM, Drag = LPM Hold
				bool isTouchPhysicallyDown = (state.bitmask_buttons & SCE_BM_TOUCH) != 0;
				auto now = std::chrono::steady_clock::now();

				if (isTouchPhysicallyDown && !ts.touchButtonPressed)
				{
					// Physical button just pressed down
					ts.touchButtonPressed = true;
					ts.touchDownTime = now;
					ts.clickDownX = static_cast<float>(state.touchData.touch[0].x);
					ts.clickDownY = static_cast<float>(state.touchData.touch[0].y);
					ts.isHoldingRightClick = false;
					ts.isLeftDragging = false;

					if (finger1Down)
					{
						// Two-finger click -> instant PPM
						ts.isTwoFingerClick = true;
						MouseClick(MOUSEEVENTF_RIGHTDOWN);
					}
					else
					{
						ts.isTwoFingerClick = false;
					}
				}
				else if (isTouchPhysicallyDown && ts.touchButtonPressed)
				{
					// Physical button is currently held down
					if (!ts.isTwoFingerClick && !ts.isHoldingRightClick)
					{
						float dx = static_cast<float>(state.touchData.touch[0].x) - ts.clickDownX;
						float dy = static_cast<float>(state.touchData.touch[0].y) - ts.clickDownY;
						float distSq = dx * dx + dy * dy;

						if (!ts.isLeftDragging && distSq > 400.0f) // moved > 20 touchpad units -> dragging
						{
							ts.isLeftDragging = true;
							MouseClick(MOUSEEVENTF_LEFTDOWN);
						}
						else if (!ts.isLeftDragging)
						{
							auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - ts.touchDownTime);
							if (elapsed >= std::chrono::milliseconds(380))
							{
								// Held stationary for >= 380ms -> trigger PPM
								ts.isHoldingRightClick = true;
								MouseClick(MOUSEEVENTF_RIGHTDOWN);
							}
						}
					}
				}
				else if (!isTouchPhysicallyDown && ts.touchButtonPressed)
				{
					// Physical button released
					ts.touchButtonPressed = false;

					if (ts.isTwoFingerClick)
					{
						MouseClick(MOUSEEVENTF_RIGHTUP);
						ts.isTwoFingerClick = false;
					}
					else if (ts.isHoldingRightClick)
					{
						MouseClick(MOUSEEVENTF_RIGHTUP);
						ts.isHoldingRightClick = false;
					}
					else if (ts.isLeftDragging)
					{
						MouseClick(MOUSEEVENTF_LEFTUP);
						ts.isLeftDragging = false;
					}
					else
					{
						// Short tap/click -> send full Left Mouse Click (DOWN + UP)
						MouseClick(MOUSEEVENTF_LEFTDOWN);
						MouseClick(MOUSEEVENTF_LEFTUP);
					}
				}
			}
#pragma endregion

#pragma region Emulate analog wsad
			if (m_ScePadSettings[i].emulateAnalogWsad)
			{
				int lx = state.LeftStick.X;
				int ly = state.LeftStick.Y;

				// W = stick up
				bool wNow = (ly < 128 - DEADZONE);
				if (wNow != wHeld)
				{
					sendKeyScan(SC_W, wNow);
					wHeld = wNow;
				}

				// S = stick down
				bool sNow = (ly > 128 + DEADZONE);
				if (sNow != sHeld)
				{
					sendKeyScan(SC_S, sNow);
					sHeld = sNow;
				}

				// A = stick left
				bool aNow = (lx < 128 - DEADZONE);
				if (aNow != aHeld)
				{
					sendKeyScan(SC_A, aNow);
					aHeld = aNow;
				}

				// D = stick right
				bool dNow = (lx > 128 + DEADZONE);
				if (dNow != dHeld)
				{
					sendKeyScan(SC_D, dNow);
					dHeld = dNow;
				}

				if (wNow && dNow && fire)
				{
					sendKeyScan(SC_D, dNow);
				}

				if (wNow && aNow && fire)
				{
					sendKeyScan(SC_A, aNow);
				}
			}
#pragma endregion

#pragma region Gyro to mouse

			if (m_ScePadSettings[i].gyroToMouse)
			{
				if (m_ScePadSettings[i].useGyroMouseHotkey && !IsHotkeyActive(m_ScePadSettings[i].gyroMouseHotkey, state.bitmask_buttons))
					goto skipGyroToMouse;

				static bool lastVelX[4] = {0};
				static bool lastVelY[4] = {0};

				float velX = -state.angularVelocity.z;
				float velY = -state.angularVelocity.x;

				float X = ((velX - lastVelX[i]) / 100.0f) * m_ScePadSettings[i].gyroToMouseSensitivity;
				float Y = ((velY - lastVelY[i]) / 100.0f) * m_ScePadSettings[i].gyroToMouseSensitivity;

				if (abs(X) > 0.1f && abs(Y) > 0.1f)
					MoveCursor(X, Y);

				lastVelX[i] = velX;
				lastVelY[i] = velY;
			}
			skipGyroToMouse:
#pragma endregion

#pragma region Mouse1 hotkey
			if (m_ScePadSettings[i].useMouse1Hotkey)
			{
				static bool wasPressed[4] = {false};

				if ((state.bitmask_buttons & m_ScePadSettings[i].mouse1Hotkey) && !wasPressed[i])
				{
					MouseClick(MOUSEEVENTF_LEFTDOWN);
					wasPressed[i] = true;
				}
				else if (!(state.bitmask_buttons & m_ScePadSettings[i].mouse1Hotkey) && wasPressed[i])
				{
					MouseClick(MOUSEEVENTF_LEFTUP);
					wasPressed[i] = false;
				}
			}

#pragma endregion

#pragma region PS button as Windows key
			if (m_ScePadSettings[i].psBtnAsWinKey)
			{
				auto now = std::chrono::steady_clock::now();
				bool psRawPressed = (state.bitmask_buttons & SCE_BM_PSBTN) != 0;
				auto& ps = m_psState[i];

				const auto DEBOUNCE_DURATION = std::chrono::milliseconds(30);
				const auto MIN_HOLD_DURATION = std::chrono::milliseconds(35);
				const auto MAX_HOLD_DURATION = std::chrono::milliseconds(5000);

				if (psRawPressed && !ps.isDown)
				{
					// Rising edge: button pressed
					if (now - ps.lastEdgeTime >= DEBOUNCE_DURATION)
					{
						sendVirtualKey(VK_LWIN, true);
						ps.isDown = true;
						ps.releasePending = false;
						ps.pressTime = now;
						ps.lastEdgeTime = now;
					}
				}
				else if (!psRawPressed && ps.isDown)
				{
					// Falling edge: button released
					if (now - ps.pressTime >= MIN_HOLD_DURATION)
					{
						if (now - ps.lastEdgeTime >= DEBOUNCE_DURATION)
						{
							sendVirtualKey(VK_LWIN, false);
							ps.isDown = false;
							ps.releasePending = false;
							ps.lastEdgeTime = now;
						}
					}
					else
					{
						// Ensure key is held for at least MIN_HOLD_DURATION so Windows registers it
						ps.releasePending = true;
					}
				}
				else if (ps.isDown && ps.releasePending)
				{
					// Minimum hold duration elapsed for quick tap
					if (now - ps.pressTime >= MIN_HOLD_DURATION)
					{
						sendVirtualKey(VK_LWIN, false);
						ps.isDown = false;
						ps.releasePending = false;
						ps.lastEdgeTime = now;
					}
				}
				else if (ps.isDown && (now - ps.pressTime >= MAX_HOLD_DURATION))
				{
					// Safety timeout: release key if held longer than 5s
					sendVirtualKey(VK_LWIN, false);
					ps.isDown = false;
					ps.releasePending = false;
					ps.lastEdgeTime = now;
				}
			}
			else if (m_psState[i].isDown)
			{
				// Safety: release if setting is toggled off while held
				sendVirtualKey(VK_LWIN, false);
				m_psState[i].isDown = false;
				m_psState[i].releasePending = false;
			}
#pragma endregion
		}

		if (fire)
		{
			lastTime = std::chrono::steady_clock::now();
		}

		SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0);
		WaitForSingleObject(hTimer, INFINITE);
	}
#endif
}

void KeyboardMouseMapper::MoveCursor(int x, int y)
{
#ifdef WINDOWS
	INPUT input = {0};
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_MOVE;
	input.mi.dx = x;
	input.mi.dy = y;

	SendInput(1, &input, sizeof(INPUT));
#endif
}

KeyboardMouseMapper::KeyboardMouseMapper(s_scePadSettings *scePadSettings) : m_ScePadSettings(scePadSettings)
{
#ifdef WINDOWS
	m_thread = std::thread(&KeyboardMouseMapper::Thread, this);
	m_thread.detach();
#endif
}

KeyboardMouseMapper::~KeyboardMouseMapper()
{
#ifdef WINDOWS
	m_ThreadRunning = false;

	for (int i = 0; i < 4; i++)
	{
		if (m_psState[i].isDown)
		{
			sendVirtualKey(VK_LWIN, false);
			m_psState[i].isDown = false;
			m_psState[i].releasePending = false;
		}
	}

	if (m_thread.joinable())
	{
		m_thread.join();
	}
#endif
}
