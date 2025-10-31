#include "controllerEmulation.hpp"
#include "log.hpp"
#include "scePadSettings.hpp"
#include "controllerHotkey.hpp"
#include <cmath>

int convertRange(int value, int oldMin, int oldMax, int newMin, int newMax) {
	if (oldMin == oldMax) {
		throw std::invalid_argument("Old minimum and maximum cannot be equal.");
	}
	float ratio = static_cast<float>(newMax - newMin) / static_cast<float>(oldMax - oldMin);
	float scaledValue = (value - oldMin) * ratio + newMin;
	return std::clamp(static_cast<int>(scaledValue), newMin, newMax);
}

#ifdef WINDOWS
PVIGEM_CLIENT Vigem::m_vigemClient;
#endif

#ifdef WINDOWS
void Vigem::update360ByTarget(PVIGEM_TARGET Target, s_ScePadData& state) {
	XUSB_REPORT report = {};

	report.wButtons |= (state.bitmask_buttons & SCE_BM_CROSS) ? XINPUT_GAMEPAD_A : 0;
	report.wButtons |= (state.bitmask_buttons & SCE_BM_CIRCLE) ? XINPUT_GAMEPAD_B : 0;
	report.wButtons |= (state.bitmask_buttons & SCE_BM_SQUARE) ? XINPUT_GAMEPAD_X : 0;
	report.wButtons |= (state.bitmask_buttons & SCE_BM_TRIANGLE) ? XINPUT_GAMEPAD_Y : 0;

	report.wButtons |= (state.bitmask_buttons & SCE_BM_N_DPAD) ? XINPUT_GAMEPAD_DPAD_UP : 0;
	report.wButtons |= (state.bitmask_buttons & SCE_BM_E_DPAD) ? XINPUT_GAMEPAD_DPAD_RIGHT : 0;
	report.wButtons |= (state.bitmask_buttons & SCE_BM_S_DPAD) ? XINPUT_GAMEPAD_DPAD_DOWN : 0;
	report.wButtons |= (state.bitmask_buttons & SCE_BM_W_DPAD) ? XINPUT_GAMEPAD_DPAD_LEFT : 0;

	report.wButtons |= (state.bitmask_buttons & SCE_BM_OPTIONS) ? XINPUT_GAMEPAD_START : 0;
	report.wButtons |= (state.bitmask_buttons & SCE_BM_TOUCH) ? XINPUT_GAMEPAD_BACK : 0;

	report.wButtons |= (state.bitmask_buttons & SCE_BM_L1) ? XINPUT_GAMEPAD_LEFT_SHOULDER : 0;
	report.wButtons |= (state.bitmask_buttons & SCE_BM_R1) ? XINPUT_GAMEPAD_RIGHT_SHOULDER : 0;

	report.wButtons |= (state.bitmask_buttons & SCE_BM_L3) ? XINPUT_GAMEPAD_LEFT_THUMB : 0;
	report.wButtons |= (state.bitmask_buttons & SCE_BM_R3) ? XINPUT_GAMEPAD_RIGHT_THUMB : 0;

	report.bLeftTrigger = state.L2_Analog;
	report.bRightTrigger = state.R2_Analog;

	report.sThumbLX = convertRange(state.LeftStick.X, 0, 255, -32767, 32766);
	report.sThumbLY = convertRange(state.LeftStick.Y, 255, 0, -32767, 32766);
	report.sThumbRX = convertRange(state.RightStick.X, 0, 255, -32767, 32766);
	report.sThumbRY = convertRange(state.RightStick.Y, 255, 0, -32767, 32766);

	vigem_target_x360_update(m_vigemClient, Target, report);
}

void Vigem::updateDs4ByTarget(PVIGEM_TARGET Target, s_ScePadData& state) {
	DS4_REPORT_EX report{};
	report.Report.bThumbLX = state.LeftStick.X;
	report.Report.bThumbLY = state.LeftStick.Y;
	report.Report.bThumbRX = state.RightStick.X;
	report.Report.bThumbRY = state.RightStick.Y;

	USHORT buttons = 0;
	buttons = state.bitmask_buttons & SCE_BM_R3 ? buttons | 1 << 15 : buttons;
	buttons = state.bitmask_buttons & SCE_BM_L3 ? buttons | 1 << 14 : buttons;
	buttons = state.bitmask_buttons & SCE_BM_OPTIONS ? buttons | 1 << 13 : buttons;
	buttons = state.bitmask_buttons & SCE_BM_SHARE ? buttons | 1 << 12 : buttons;
	buttons = state.bitmask_buttons & SCE_BM_R2 ? buttons | 1 << 11 : buttons;
	buttons = state.bitmask_buttons & SCE_BM_L2 ? buttons | 1 << 10 : buttons;
	buttons = state.bitmask_buttons & SCE_BM_R1 ? buttons | 1 << 9 : buttons;
	buttons = state.bitmask_buttons & SCE_BM_L1 ? buttons | 1 << 8 : buttons;
	buttons = state.bitmask_buttons & SCE_BM_TRIANGLE ? buttons | 1 << 7 : buttons;
	buttons = state.bitmask_buttons & SCE_BM_CIRCLE ? buttons | 1 << 6 : buttons;
	buttons = state.bitmask_buttons & SCE_BM_CROSS ? buttons | 1 << 5 : buttons;
	buttons = state.bitmask_buttons & SCE_BM_SQUARE ? buttons | 1 << 4 : buttons;

	if (!(state.bitmask_buttons & SCE_BM_N_DPAD) && !(state.bitmask_buttons & SCE_BM_S_DPAD) && !(state.bitmask_buttons & SCE_BM_W_DPAD) && !(state.bitmask_buttons & SCE_BM_E_DPAD))
		buttons |= 0x8;
	else {
		buttons &= ~0xF;
		if ((state.bitmask_buttons & SCE_BM_S_DPAD) && (state.bitmask_buttons & SCE_BM_W_DPAD)) buttons |= (USHORT)DS4_BUTTON_DPAD_SOUTHWEST;
		else if (state.bitmask_buttons & SCE_BM_S_DPAD && state.bitmask_buttons & SCE_BM_E_DPAD) buttons |= (USHORT)DS4_BUTTON_DPAD_SOUTHEAST;
		else if (state.bitmask_buttons & SCE_BM_N_DPAD && state.bitmask_buttons & SCE_BM_E_DPAD) buttons |= (USHORT)DS4_BUTTON_DPAD_NORTHEAST;
		else if (state.bitmask_buttons & SCE_BM_N_DPAD && state.bitmask_buttons & SCE_BM_W_DPAD) buttons |= (USHORT)DS4_BUTTON_DPAD_NORTHWEST;
		else if (state.bitmask_buttons & SCE_BM_N_DPAD) buttons |= (USHORT)DS4_BUTTON_DPAD_NORTH;
		else if (state.bitmask_buttons & SCE_BM_E_DPAD) buttons |= (USHORT)DS4_BUTTON_DPAD_EAST;
		else if (state.bitmask_buttons & SCE_BM_S_DPAD) buttons |= (USHORT)DS4_BUTTON_DPAD_SOUTH;
		else if (state.bitmask_buttons & SCE_BM_W_DPAD) buttons |= (USHORT)DS4_BUTTON_DPAD_WEST;
	}
	report.Report.wButtons = buttons;

	USHORT specialbuttons = 0;
	specialbuttons = state.bitmask_buttons & SCE_BM_PSBTN ? specialbuttons | 1 << 0 : specialbuttons;
	specialbuttons = state.bitmask_buttons & SCE_BM_TOUCH ? specialbuttons | 1 << 1 : specialbuttons;
	report.Report.bSpecial = specialbuttons;

	report.Report.bTriggerL = state.L2_Analog;
	report.Report.bTriggerR = state.R2_Analog;
	report.Report.bBatteryLvl = 100;

	report.Report.sPreviousTouch[0] = report.Report.sCurrentTouch;

	static int packetNum = 0;
	packetNum++;
	if (packetNum > 255) packetNum = 0;

	DS4_TOUCH touch{};
	touch.bPacketCounter = packetNum;

	touch.bIsUpTrackingNum1 = state.touchData.touch[0].reserve[0] == 0 ? state.touchData.touch[0].id : (state.touchData.touch[0].id | 0x80);
	touch.bTouchData1[0] = state.touchData.touch[0].x & 0xFF;
	touch.bTouchData1[1] = state.touchData.touch[0].x >> 8 & 0x0F | state.touchData.touch[0].y << 4 & 0xF0;
	touch.bTouchData1[2] = state.touchData.touch[0].y >> 4;

	touch.bIsUpTrackingNum2 = state.touchData.touch[1].reserve[0] == 0 ? state.touchData.touch[1].id : (state.touchData.touch[1].id | 0x80);
	touch.bTouchData2[0] = state.touchData.touch[1].x & 0xFF;
	touch.bTouchData2[1] = state.touchData.touch[1].x >> 8 & 0x0F | state.touchData.touch[1].y << 4 & 0xF0;
	touch.bTouchData2[2] = state.touchData.touch[1].y >> 4;
	report.Report.sCurrentTouch = touch;

	report.Report.wAccelX = state.acceleration.x;
	report.Report.wAccelY = state.acceleration.y;
	report.Report.wAccelZ = state.acceleration.z;
	report.Report.wGyroX = state.angularVelocity.x;
	report.Report.wGyroY = state.angularVelocity.y;
	report.Report.wGyroZ = state.angularVelocity.z;
	report.Report.wTimestamp = state.timestamp / 16;

	vigem_target_ds4_update_ex(m_vigemClient, Target, report);
}
#endif

Vigem::Vigem(s_scePadSettings* scePadSettings, UDP& udp) : m_scePadSettings(scePadSettings), m_udp(udp) {
#ifdef WINDOWS
	if (!m_vigemClientInitalized) {
		m_vigemClient = vigem_alloc();

		VIGEM_ERROR retval = vigem_connect(m_vigemClient);
		if (!VIGEM_SUCCESS(retval)) {
			LOGE("Failed to connect to ViGEm client");
			return;
		}

		m_vigemThread = std::thread(&Vigem::emulatedControllerUpdate, this);
		m_vigemThread.detach();

		LOGI("ViGEm Client initialized");
		m_vigemClientInitalized = true;
	}

	for (uint32_t i = 0; i < 4; i++) {
		m_360[i] = vigem_target_x360_alloc();
		m_ds4[i] = vigem_target_ds4_alloc();
	}
#endif
}

Vigem::~Vigem() {
#ifdef WINDOWS
	m_vigemThreadRunning = false;
	if (m_vigemThread.joinable()) {
		m_vigemThread.join();
	}

	for (uint32_t i = 0; i < 4; i++) {
		vigem_target_x360_unregister_notification(m_360[i]);
		vigem_target_remove(m_vigemClient, m_360[i]);
		vigem_target_ds4_unregister_notification(m_ds4[i]);
		vigem_target_remove(m_vigemClient, m_ds4[i]);
		vigem_target_free(m_360[i]);
		vigem_target_free(m_ds4[i]);
	}

	vigem_disconnect(m_vigemClient);
	vigem_free(m_vigemClient);
#endif
}

void Vigem::plugControllerByIndex(uint32_t index, uint32_t controllerType) {
#ifdef WINDOWS
	static uint32_t lastEmulatedController[4] = {};

	if ((EmulatedController)controllerType == EmulatedController::NONE && (EmulatedController)lastEmulatedController[index] != EmulatedController::NONE) {
		vigem_target_remove(m_vigemClient, m_360[index]);
		vigem_target_remove(m_vigemClient, m_ds4[index]);
		vigem_target_x360_unregister_notification(m_360[index]);
		vigem_target_ds4_unregister_notification(m_ds4[index]);
		lastEmulatedController[index] = (uint32_t)EmulatedController::NONE;
	}
	else if ((EmulatedController)controllerType == EmulatedController::XBOX360 && (EmulatedController)lastEmulatedController[index] != EmulatedController::XBOX360) {
		vigem_target_remove(m_vigemClient, m_ds4[index]);
		vigem_target_ds4_unregister_notification(m_ds4[index]);
		vigem_target_add(m_vigemClient, m_360[index]);
		vigem_target_x360_register_notification(m_vigemClient, m_360[index], xbox360Notification, &m_userData[index]);
		lastEmulatedController[index] = (uint32_t)EmulatedController::XBOX360;
	}
	else if ((EmulatedController)controllerType == EmulatedController::DUALSHOCK4 && (EmulatedController)lastEmulatedController[index] != EmulatedController::DUALSHOCK4) {
		vigem_target_remove(m_vigemClient, m_360[index]);
		vigem_target_x360_unregister_notification(m_360[index]);
		vigem_target_add(m_vigemClient, m_ds4[index]);
		vigem_target_ds4_register_notification(m_vigemClient, m_ds4[index], ds4Notification, &m_userData[index]);
		lastEmulatedController[index] = (uint32_t)EmulatedController::DUALSHOCK4;
	}
#endif
}

void Vigem::setSelectedController(uint32_t selectedController) {
	m_selectedController = selectedController;
}

void Vigem::SetPeerControllerDataPointer(std::shared_ptr<std::unordered_map<uint32_t, PeerControllerData>> Pointer) {
#ifdef WINDOWS
	m_PeerControllers = Pointer;
#endif
}

bool Vigem::isVigemConnected() {
#if !defined(__linux__) && !defined(__MACOS__)
	return m_vigemClientInitalized;
#endif;
	return false;
}

void Vigem::applyInputSettingsToScePadState(s_scePadSettings& settings, s_ScePadData& state) {
#pragma region Trigger threshold
	state.L2_Analog = state.L2_Analog >= settings.leftTriggerThreshold ? state.L2_Analog : 0;
	state.R2_Analog = state.R2_Analog >= settings.rightTriggerThreshold ? state.R2_Analog : 0;
#pragma endregion

#pragma region Analog deadzone
	auto applyDeadzone = [&](int deadzone, s_SceStickData& stick) {
		if (deadzone <= 0)
			return;

		float centerX = (stick.X - 128);
		float centerY = (stick.Y - 128);
		float magnitude = sqrt(centerX * centerX + centerY * centerY);

		float deadzoneNorm = deadzone / 128;

		stick.X = magnitude > deadzone ? stick.X : 128;
		stick.Y = magnitude > deadzone ? stick.Y : 128;
		};
	applyDeadzone(settings.leftStickDeadzone, state.LeftStick);
	applyDeadzone(settings.rightStickDeadzone, state.RightStick);

#pragma endregion

#pragma region Gyro to right stick
	if (settings.gyroToRightStick && IsHotkeyActive(settings.gyroToRightStickActivationButton, state.bitmask_buttons)) {
		if (abs(state.RightStick.X - 128) <= 80 &&
		    abs(state.RightStick.Y - 128) <= 80) {

			const float maxVelValue = 1000.0f;

			float normalizedVelX = state.angularVelocity.x / maxVelValue;
			float normalizedVelY = state.angularVelocity.z / maxVelValue;

			float adjustedX = -normalizedVelX * settings.gyroToRightStickSensitivity;
			float adjustedY = -normalizedVelY * settings.gyroToRightStickSensitivity;

			state.RightStick.X = static_cast<int>((adjustedY) - 127);
			state.RightStick.Y = static_cast<int>((adjustedX) - 127);
			applyDeadzone(settings.gyroToRightStickDeadzone, state.RightStick);
		}
	}
#pragma endregion
}

#ifdef WINDOWS
void Vigem::emulatedControllerUpdate() {
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	timeBeginPeriod(1);

	EXECUTION_STATE prevState = SetThreadExecutionState(
		ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED
	);

	HANDLE hTimer = CreateWaitableTimerEx(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
	LARGE_INTEGER liDueTime;
	liDueTime.QuadPart = -5000LL;

	while (m_vigemThreadRunning) {
		for (uint32_t i = 0; i < 4; i++) {

			if ((EmulatedController)m_scePadSettings[i].emulatedController != EmulatedController::NONE) {
				s_ScePadData scePadState = {};
				uint32_t result = scePadReadState(g_scePad[i], &scePadState);

				s_scePadSettings settingsToUse = (m_selectedController == i && m_udp.isActive()) ? m_udp.getSettings() : m_scePadSettings[i];
				applyInputSettingsToScePadState(settingsToUse, scePadState);

				if (result == SCE_OK) {

					if ((EmulatedController)m_scePadSettings[i].emulatedController == EmulatedController::XBOX360) {
						update360ByTarget(m_360[i], scePadState);
					}
					else if ((EmulatedController)m_scePadSettings[i].emulatedController == EmulatedController::DUALSHOCK4) {
						updateDs4ByTarget(m_ds4[i], scePadState);
					}
				}
			}
		}

		if (auto peerControllers = m_PeerControllers.lock()) {
			for (auto it = peerControllers->begin(); it != peerControllers->end(); ) {
				auto& peer = it->second;
				auto targetIter = m_PeerControllerTargets.find(it->first);

				if (peer.AllowedToReceive && targetIter == m_PeerControllerTargets.end()) {
					PVIGEM_TARGET target = peer.Controller == CONTROLLER::XBOX360 ? vigem_target_x360_alloc() : vigem_target_ds4_alloc();
					VIGEM_ERROR error = vigem_target_add(m_vigemClient, target);
					if (peer.Controller == CONTROLLER::XBOX360) vigem_target_x360_register_notification(m_vigemClient, target, x360PeerNotification, &peer);
					if (peer.Controller == CONTROLLER::DUALSHOCK4) vigem_target_ds4_register_notification(m_vigemClient, target, ds4PeerNotification, &peer);
					if (error == VIGEM_ERROR_NONE) {
						m_PeerControllerTargets[it->first] = target;
						it->second.Disconnected = false;
						LOGI("[PEER CONTROLLER] Controller created");
					}
					else {
						LOGE("[PEER CONTROLLER] Failed to allocate peer controller");
					}
				}

				if (targetIter != m_PeerControllerTargets.end() && peer.Disconnected) {
					if (!targetIter->second) continue;
					VIGEM_ERROR error = vigem_target_remove(m_vigemClient, targetIter->second);
					if (error == VIGEM_ERROR_NONE) {
						if (peer.Controller == CONTROLLER::XBOX360) vigem_target_x360_unregister_notification(targetIter->second);
						else if (peer.Controller == CONTROLLER::DUALSHOCK4) vigem_target_ds4_unregister_notification(targetIter->second);
						vigem_target_free(targetIter->second);
						m_PeerControllerTargets.erase(targetIter);
						it->second.AllowedToReceive = false;
						it->second.AllowedToSend = false;
						it->second.Disconnected = false;
						it = peerControllers->erase(it);
						LOGI("[PEER CONTROLLER] Controller destroyed");
					}
					else {
						LOGE("[PEER CONTROLLER] Controller failed to be removed, ERROR: %d", (int)error);
					}
					continue;
				}

				s_ScePadData inputState = {  };
				inputState.LeftStick.X = 128; inputState.LeftStick.Y = 128;
				inputState.RightStick.X = 128; inputState.RightStick.Y = 128;
				{
					std::lock_guard<std::mutex> inputGuard(peer.Lock);
					inputState = peer.InputState;
				}
				applyInputSettingsToScePadState(peer.Settings, inputState);

				targetIter = m_PeerControllerTargets.find(it->first);
				if (targetIter != m_PeerControllerTargets.end() && targetIter->second) {
					if(peer.Controller == CONTROLLER::XBOX360)
						update360ByTarget(targetIter->second, inputState);
					else if (peer.Controller == CONTROLLER::DUALSHOCK4)
						updateDs4ByTarget(targetIter->second, inputState);
				}

				++it;
			}
		}

		SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0);
		WaitForSingleObject(hTimer, INFINITE);
	}

	CloseHandle(hTimer);
}
#endif

#ifdef WINDOWS
VOID Vigem::xbox360Notification(PVIGEM_CLIENT Client, PVIGEM_TARGET Target, UCHAR LargeMotor, UCHAR SmallMotor, UCHAR LedNumber, LPVOID UserData) {
	auto* data = static_cast<VigemUserData*>(UserData);
	if (!data || !data->instance) return;
	if (!data->instance->m_scePadSettings) return;

	data->instance->m_scePadSettings[data->index].rumbleFromEmulatedController = { LargeMotor, SmallMotor };
}

VOID Vigem::ds4Notification(PVIGEM_CLIENT Client, PVIGEM_TARGET Target, UCHAR LargeMotor, UCHAR SmallMotor, DS4_LIGHTBAR_COLOR LightbarColor, LPVOID UserData) {
	auto* data = static_cast<VigemUserData*>(UserData);
	if (!data || !data->instance) return;
	if (!data->instance->m_scePadSettings) return;

	data->instance->m_scePadSettings[data->index].lightbarFromEmulatedController = { LightbarColor.Red, LightbarColor.Green, LightbarColor.Blue };	
	data->instance->m_scePadSettings[data->index].rumbleFromEmulatedController = { LargeMotor, SmallMotor };
}
VOID Vigem::x360PeerNotification(PVIGEM_CLIENT Client, PVIGEM_TARGET Target, UCHAR LargeMotor, UCHAR SmallMotor, UCHAR LedNumber, LPVOID UserData) {
	auto* data = static_cast<PeerControllerData*>(UserData);
	if (!data) return;

	data->Vibration = { LargeMotor, SmallMotor };
}
VOID Vigem::ds4PeerNotification(PVIGEM_CLIENT Client, PVIGEM_TARGET Target, UCHAR LargeMotor, UCHAR SmallMotor, DS4_LIGHTBAR_COLOR LightbarColor, LPVOID UserData) {
	auto* data = static_cast<PeerControllerData*>(UserData);
	if (!data) return;

	data->Vibration = { LargeMotor, SmallMotor };
	data->Lightbar = {LightbarColor.Red, LightbarColor.Green, LightbarColor.Blue};
}
#endif

