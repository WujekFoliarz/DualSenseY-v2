#include "controllerEmulation.hpp"
#include "log.hpp"
#include "scePadSettings.hpp"

int convertRange(int value, int oldMin, int oldMax, int newMin, int newMax) {
	if (oldMin == oldMax) {
		throw std::invalid_argument("Old minimum and maximum cannot be equal.");
	}
	float ratio = static_cast<float>(newMax - newMin) / static_cast<float>(oldMax - oldMin);
	float scaledValue = (value - oldMin) * ratio + newMin;
	return std::clamp(static_cast<int>(scaledValue), newMin, newMax);
}

#if !defined(__linux__) && !defined(__MACOS__)
PVIGEM_CLIENT Vigem::m_vigemClient;
#endif

void Vigem::update360ByIndex(uint32_t index, s_ScePadData& state) {
#if !defined(__linux__) && !defined(__MACOS__)
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

	vigem_target_x360_update(m_vigemClient, m_360[index], report);
#endif
}

void Vigem::updateDs4ByIndex(uint32_t index, s_ScePadData& state) {
#if !defined(__linux__) && !defined(__MACOS__)
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

	DS4_TOUCH touch;
	//touch.bPacketCounter = state.TouchPacketNum;
	//report.Report.bTouchPacketsN = state.TouchPacketNum;
	//touch.bIsUpTrackingNum1 = state.trackPadTouch0.RawTrackingNum;
	//touch.bTouchData1[0] = state.trackPadTouch0.X & 0xFF;
	//touch.bTouchData1[1] = state.trackPadTouch0.X >> 8 & 0x0F | state.trackPadTouch0.Y << 4 & 0xF0;
	//touch.bTouchData1[2] = state.trackPadTouch0.Y >> 4;

	//touch.bIsUpTrackingNum2 = state.trackPadTouch1.RawTrackingNum;
	//touch.bTouchData2[0] = state.trackPadTouch1.X & 0xFF;
	//touch.bTouchData2[1] = state.trackPadTouch1.X >> 8 & 0x0F | state.trackPadTouch1.Y << 4 & 0xF0;
	//touch.bTouchData2[2] = state.trackPadTouch1.Y >> 4;
	//report.Report.sCurrentTouch = touch;

	//report.Report.wAccelX = state.gyro.X;
	//report.Report.wAccelY = state.gyro.Y;
	//report.Report.wAccelZ = state.gyro.Z;
	//report.Report.wGyroX = state.accelerometer.X;
	//report.Report.wGyroY = state.accelerometer.Y;
	//report.Report.wGyroZ = state.accelerometer.Z;
	//report.Report.wTimestamp = state.accelerometer.SensorTimestamp != 0 ? state.accelerometer.SensorTimestamp / 16 : 0;

	vigem_target_ds4_update_ex(m_vigemClient, m_ds4[index], report);
#endif
}

Vigem::Vigem() {
#if !defined(__linux__) && !defined(__MACOS__)
	if (!m_vigemClientInitalized) {
		m_vigemClient = vigem_alloc();

		VIGEM_ERROR retval = vigem_connect(m_vigemClient);
		if (!VIGEM_SUCCESS(retval)) {
			LOGE("Failed to connect to ViGEm client");
			return;
		}

		LOGI("ViGEm Client initialized");
		m_vigemClientInitalized = true;
	}

	for (uint32_t i = 0; i < 4; i++) {
		m_360[i] = vigem_target_x360_alloc();
		m_ds4[i] = vigem_target_ds4_alloc();
	}
#endif
}

void Vigem::plugControllerByIndex(uint32_t index, uint32_t controllerType) {
#if !defined(__linux__) && !defined(__MACOS__)
	static uint32_t lastEmulatedController[4] = {};

	if ((EmulatedController)controllerType == EmulatedController::NONE && (EmulatedController)lastEmulatedController[index] != EmulatedController::NONE) {
		vigem_target_remove(m_vigemClient, m_360[index]);
		vigem_target_remove(m_vigemClient, m_ds4[index]);
		lastEmulatedController[index] = (uint32_t)EmulatedController::NONE;
	}
	else if ((EmulatedController)controllerType == EmulatedController::XBOX360 && (EmulatedController)lastEmulatedController[index] != EmulatedController::XBOX360) {
		vigem_target_remove(m_vigemClient, m_ds4[index]);
		vigem_target_add(m_vigemClient, m_360[index]);
		lastEmulatedController[index] = (uint32_t)EmulatedController::XBOX360;
	}
	else if ((EmulatedController)controllerType == EmulatedController::DUALSHOCK4 && (EmulatedController)lastEmulatedController[index] != EmulatedController::DUALSHOCK4) {
		vigem_target_remove(m_vigemClient, m_360[index]);
		vigem_target_add(m_vigemClient, m_ds4[index]);
		lastEmulatedController[index] = (uint32_t)EmulatedController::DUALSHOCK4;
	}
#endif
}

bool Vigem::isVigemConnected() {
#if !defined(__linux__) && !defined(__MACOS__)
	return m_vigemClientInitalized;
#endif;
	return false;
}