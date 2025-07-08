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

PVIGEM_CLIENT Vigem::m_vigemClient;

void Vigem::update360ByIndex(uint32_t index, s_ScePadData& state) {
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
}

Vigem::Vigem() {
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
}

void Vigem::plugControllerByIndex(uint32_t index, uint32_t controllerType) {
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
}

bool Vigem::isVigemConnected() {
	return m_vigemClientInitalized;
}