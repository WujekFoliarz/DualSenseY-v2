#include "udp.hpp"
#include "log.hpp"
#include <duaLib.h>
#include <iomanip>
#include "scePadHandle.hpp"
#include "scePadCustomTriggers.hpp"

std::string getFormattedDateTime() {
	auto now = std::chrono::system_clock::now();
	std::time_t now_time = std::chrono::system_clock::to_time_t(now);

	std::tm local_tm{};
#if !defined(__linux__) && !defined(__APPLE__)
	localtime_s(&local_tm, &now_time);
#else
	localtime_r(&now_time, &local_tm);
#endif

	std::ostringstream oss;
	oss << std::put_time(&local_tm, "%Y/%m/%d %I:%M:%S %p");
	return oss.str();
}

void UDP::listen() {
	while (m_threadRunning) {
		char buffer[1024] = {};
		asio::ip::udp::endpoint senderEndpoint;
		try {
			size_t length = m_socket.receive_from(asio::buffer(buffer), senderEndpoint);
			LOGI("[UDP] Received packet with length %d", length);
			LOGI("[UDP] Raw packet:\n%s", buffer);
			nlohmann::json packetJson = nlohmann::json::parse(buffer);
			Packet packet = {};
			packet.from_json(packetJson);

			for (auto& instr : packet.instructions) {
				// I don't care enough to implement the rest, if you wanna do it go ahead.
				switch (instr.type) {
					case InstructionType::GetDSXStatus:
						break;
					case InstructionType::TriggerUpdate:
						handleTriggerUpdate(instr);
						break;
					case InstructionType::RGBUpdate:
						handleRgbUpdate(instr);
						break;
					case InstructionType::PlayerLED:
						break;
					case InstructionType::TriggerThreshold:
						handleTriggerThresholdUpdate(instr);
						break;
					case InstructionType::MicLED:
						break;
					case InstructionType::PlayerLEDNewRevision:
						break;
					case InstructionType::ResetToUserSettings:
						break;
				}

				LOGI("[UDP] Instruction type: %d", instr.type);
			}

			m_lastUpdate = std::chrono::steady_clock::now();

			ServerResponse response = {};
			response.status = "DSX Received UDP Instructions";
			response.timeReceived = getFormattedDateTime();
			response.batteryLevel = 100;

			for (uint32_t i = 0; i < 4; i++) {
				s_SceControllerType controllerType = {};
				s_ScePadInfo scePadInfo = {};
				int busType = 0;

				scePadGetControllerType(g_scePad[i], &controllerType);
				scePadGetControllerInformation(g_scePad[i], &scePadInfo);
				uint32_t result = scePadGetControllerBusType(g_scePad[i], &busType);

				if (result == SCE_OK) {
					response.isControllerConnected = true;

					Device device = {};
					device.index = i + 1;
					device.macAddress = scePadGetMacAddress(g_scePad[i]);
					device.deviceType = controllerType == s_SceControllerType::DUALSENSE ? DeviceType::DUALSENSE : DeviceType::DUALSHOCK_V2;
					device.connectionType = (ConnectionType)(busType - 1);
					device.batteryLevel = 100;
					device.isSupportAT = controllerType == s_SceControllerType::DUALSENSE ? true : false;
					device.isSupportLightBar = true;
					device.isSupportPlayerLED = controllerType == s_SceControllerType::DUALSENSE ? true : false;
					device.isSupportMicLED = controllerType == s_SceControllerType::DUALSENSE ? true : false;

					response.devices.push_back(device);
				}
			}

			m_socket.send_to(asio::buffer(response.to_json().dump(4)), senderEndpoint);
		}
		catch (std::exception& e) {
			LOGE("[UDP] %s", e.what());
		}
	}
}

void UDP::handleRgbUpdate(Instruction instruction) {
	std::lock_guard<std::mutex> guard(m_settingsLock);
	if (instruction.parameters.size() < 4) return;

	m_settings.led[0] = static_cast<float>(std::any_cast<int>(instruction.parameters[1])) / 255.0f;
	m_settings.led[1] = static_cast<float>(std::any_cast<int>(instruction.parameters[2])) / 255.0f;
	m_settings.led[2] = static_cast<float>(std::any_cast<int>(instruction.parameters[3])) / 255.0f;
}

void UDP::handleTriggerUpdate(Instruction instruction) {
	if (instruction.parameters.size() < 3) return;
	uint32_t settingsCount = instruction.parameters.size() - 3;

	Trigger trigger = (Trigger)std::any_cast<int>(instruction.parameters[1]);
	TriggerMode triggerMode = (TriggerMode)std::any_cast<int>(instruction.parameters[2]);

	std::vector<uint8_t> settings;
	if (settingsCount > 0) {
		for (uint8_t i = 3; i < instruction.parameters.size(); i++) {
			settings.push_back((uint8_t)std::any_cast<int>(instruction.parameters[i]));
		}
	}

	bool usingDsxTrigger = false;
	if (triggerMode == TriggerMode::FEEDBACK ||
	triggerMode == TriggerMode::WEAPON ||
	triggerMode == TriggerMode::VIBRATION ||
	triggerMode == TriggerMode::SLOPE_FEEDBACK ||
	triggerMode == TriggerMode::MULTIPLE_POSITION_FEEDBACK ||
	triggerMode == TriggerMode::MULTIPLE_POSITION_VIBRATION) {
		usingDsxTrigger = false;
	}
	else {
		usingDsxTrigger = true;
	}

	if (trigger == Trigger::Left)
		m_settings.isLeftUsingDsxTrigger = usingDsxTrigger;
	else if (trigger == Trigger::Right)
		m_settings.isRightUsingDsxTrigger = usingDsxTrigger;

	switch (triggerMode) {
		case TriggerMode::Normal:
			customTriggerNormal(trigger == Trigger::Left ? m_settings.leftCustomTrigger : m_settings.rightCustomTrigger);
			break;
		case TriggerMode::GameCube:
			customTriggerGamecube(trigger == Trigger::Left ? m_settings.leftCustomTrigger : m_settings.rightCustomTrigger);
			break;
		case TriggerMode::VerySoft:
			customTriggerVerySoft(trigger == Trigger::Left ? m_settings.leftCustomTrigger : m_settings.rightCustomTrigger);
			break;
		case TriggerMode::Soft:
			customTriggerSoft(trigger == Trigger::Left ? m_settings.leftCustomTrigger : m_settings.rightCustomTrigger);
			break;
		case TriggerMode::Hard:
			customTriggerHard(trigger == Trigger::Left ? m_settings.leftCustomTrigger : m_settings.rightCustomTrigger);
			break;
		case TriggerMode::VeryHard:
			customTriggerVeryHard(trigger == Trigger::Left ? m_settings.leftCustomTrigger : m_settings.rightCustomTrigger);
			break;
		case TriggerMode::Hardest:
			customTriggerHardest(trigger == Trigger::Left ? m_settings.leftCustomTrigger : m_settings.rightCustomTrigger);
			break;
		case TriggerMode::Rigid:
			customTriggerRigid(trigger == Trigger::Left ? m_settings.leftCustomTrigger : m_settings.rightCustomTrigger);
			break;
		case TriggerMode::VibrateTrigger:
			customTriggerVibrateTrigger(trigger == Trigger::Left ? m_settings.leftCustomTrigger : m_settings.rightCustomTrigger);
			break;
		case TriggerMode::Choppy:
			customTriggerChoppy(trigger == Trigger::Left ? m_settings.leftCustomTrigger : m_settings.rightCustomTrigger);
			break;
		case TriggerMode::Medium:
			customTriggerMedium(trigger == Trigger::Left ? m_settings.leftCustomTrigger : m_settings.rightCustomTrigger);
			break;
		case TriggerMode::VibrateTriggerPulse:
			customTriggerVibrateTriggerPulse(trigger == Trigger::Left ? m_settings.leftCustomTrigger : m_settings.rightCustomTrigger);
			break;
		case TriggerMode::CustomTriggerValue:
			customTriggerCustomTriggerValue(settings, trigger == Trigger::Left ? m_settings.leftCustomTrigger : m_settings.rightCustomTrigger);
			break;
		case TriggerMode::Resistance:
			customTriggerResistance(settings, trigger == Trigger::Left ? m_settings.leftCustomTrigger : m_settings.rightCustomTrigger);
			break;
		case TriggerMode::Bow:
			customTriggerBow(settings, trigger == Trigger::Left ? m_settings.leftCustomTrigger : m_settings.rightCustomTrigger);
			break;
		case TriggerMode::Galloping:
			customTriggerGalloping(settings, trigger == Trigger::Left ? m_settings.leftCustomTrigger : m_settings.rightCustomTrigger);
			break;
		case TriggerMode::SemiAutomaticGun:
			customTriggerSemiAutomaticGun(settings, trigger == Trigger::Left ? m_settings.leftCustomTrigger : m_settings.rightCustomTrigger);
			break;
		case TriggerMode::AutomaticGun:
			customTriggerAutomaticGun(settings, trigger == Trigger::Left ? m_settings.leftCustomTrigger : m_settings.rightCustomTrigger);
			break;
		case TriggerMode::Machine:
			customTriggerMachine(settings, trigger == Trigger::Left ? m_settings.leftCustomTrigger : m_settings.rightCustomTrigger);
			break;
		case TriggerMode::VIBRATE_TRIGGER_10Hz:
			customTriggerVIBRATE_TRIGGER_10Hz(settings, trigger == Trigger::Left ? m_settings.leftCustomTrigger : m_settings.rightCustomTrigger);
			break;

		// Sony triggers
		case TriggerMode::OFF:
		{
			uint8_t index = trigger == Trigger::Left ? SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2 : SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_R2;
			m_settings.stockTriggerParam.command[index].mode = SCE_PAD_TRIGGER_EFFECT_MODE_OFF;
			break;
		}
		case TriggerMode::FEEDBACK:
		{
			if (settings.size() < 2) break;

			uint8_t index = trigger == Trigger::Left ? SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2 : SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_R2;
			m_settings.stockTriggerParam.command[index].mode = SCE_PAD_TRIGGER_EFFECT_MODE_FEEDBACK;
			m_settings.stockTriggerParam.command[index].commandData.feedbackParam.position = settings[0];
			m_settings.stockTriggerParam.command[index].commandData.feedbackParam.strength = settings[1];
			break;
		}
		case TriggerMode::WEAPON:
		{
			if (settings.size() < 3) break;

			uint8_t index = trigger == Trigger::Left ? SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2 : SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_R2;
			m_settings.stockTriggerParam.command[index].mode = SCE_PAD_TRIGGER_EFFECT_MODE_WEAPON;
			m_settings.stockTriggerParam.command[index].commandData.weaponParam.startPosition = settings[0];
			m_settings.stockTriggerParam.command[index].commandData.weaponParam.endPosition = settings[1];
			m_settings.stockTriggerParam.command[index].commandData.weaponParam.strength = settings[2];
			break;
		}
		case TriggerMode::VIBRATION:
		{
			if (settings.size() < 3) break;

			uint8_t index = trigger == Trigger::Left ? SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2 : SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_R2;
			m_settings.stockTriggerParam.command[index].mode = SCE_PAD_TRIGGER_EFFECT_MODE_VIBRATION;
			m_settings.stockTriggerParam.command[index].commandData.vibrationParam.position = settings[0];
			m_settings.stockTriggerParam.command[index].commandData.vibrationParam.amplitude = settings[1];
			m_settings.stockTriggerParam.command[index].commandData.vibrationParam.frequency = settings[2];
			break;
		}
		case TriggerMode::SLOPE_FEEDBACK:
		{
			if (settings.size() < 4) break;

			uint8_t index = trigger == Trigger::Left ? SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2 : SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_R2;
			m_settings.stockTriggerParam.command[index].mode = SCE_PAD_TRIGGER_EFFECT_MODE_SLOPE_FEEDBACK;
			m_settings.stockTriggerParam.command[index].commandData.slopeFeedbackParam.startPosition = settings[0];
			m_settings.stockTriggerParam.command[index].commandData.slopeFeedbackParam.endPosition = settings[1];
			m_settings.stockTriggerParam.command[index].commandData.slopeFeedbackParam.startStrength = settings[2];
			m_settings.stockTriggerParam.command[index].commandData.slopeFeedbackParam.endStrength = settings[3];
			break;
		}
		case TriggerMode::MULTIPLE_POSITION_FEEDBACK:
		{
			if (settings.size() < 10) break;

			uint8_t index = trigger == Trigger::Left ? SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2 : SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_R2;
			m_settings.stockTriggerParam.command[index].mode = SCE_PAD_TRIGGER_EFFECT_MODE_MULTIPLE_POSITION_FEEDBACK;
			m_settings.stockTriggerParam.command[index].commandData.multiplePositionFeedbackParam.strength[0] = settings[0];
			m_settings.stockTriggerParam.command[index].commandData.multiplePositionFeedbackParam.strength[1] = settings[1];
			m_settings.stockTriggerParam.command[index].commandData.multiplePositionFeedbackParam.strength[2] = settings[2];
			m_settings.stockTriggerParam.command[index].commandData.multiplePositionFeedbackParam.strength[3] = settings[3];
			m_settings.stockTriggerParam.command[index].commandData.multiplePositionFeedbackParam.strength[4] = settings[4];
			m_settings.stockTriggerParam.command[index].commandData.multiplePositionFeedbackParam.strength[5] = settings[5];
			m_settings.stockTriggerParam.command[index].commandData.multiplePositionFeedbackParam.strength[6] = settings[6];
			m_settings.stockTriggerParam.command[index].commandData.multiplePositionFeedbackParam.strength[7] = settings[7];
			m_settings.stockTriggerParam.command[index].commandData.multiplePositionFeedbackParam.strength[8] = settings[8];
			m_settings.stockTriggerParam.command[index].commandData.multiplePositionFeedbackParam.strength[9] = settings[9];
			break;
		}
		case TriggerMode::MULTIPLE_POSITION_VIBRATION:
		{
			if (settings.size() < 11) break;

			uint8_t index = trigger == Trigger::Left ? SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2 : SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_R2;
			m_settings.stockTriggerParam.command[index].mode = SCE_PAD_TRIGGER_EFFECT_MODE_MULTIPLE_POSITION_VIBRATION;
			m_settings.stockTriggerParam.command[index].commandData.multiplePositionVibrationParam.frequency = settings[0];
			m_settings.stockTriggerParam.command[index].commandData.multiplePositionVibrationParam.amplitude[0] = settings[1];
			m_settings.stockTriggerParam.command[index].commandData.multiplePositionVibrationParam.amplitude[1] = settings[2];
			m_settings.stockTriggerParam.command[index].commandData.multiplePositionVibrationParam.amplitude[2] = settings[3];
			m_settings.stockTriggerParam.command[index].commandData.multiplePositionVibrationParam.amplitude[3] = settings[4];
			m_settings.stockTriggerParam.command[index].commandData.multiplePositionVibrationParam.amplitude[4] = settings[5];
			m_settings.stockTriggerParam.command[index].commandData.multiplePositionVibrationParam.amplitude[5] = settings[6];
			m_settings.stockTriggerParam.command[index].commandData.multiplePositionVibrationParam.amplitude[6] = settings[7];
			m_settings.stockTriggerParam.command[index].commandData.multiplePositionVibrationParam.amplitude[7] = settings[8];
			m_settings.stockTriggerParam.command[index].commandData.multiplePositionVibrationParam.amplitude[8] = settings[9];
			m_settings.stockTriggerParam.command[index].commandData.multiplePositionVibrationParam.amplitude[9] = settings[10];
			break;
		}
	}
}

void UDP::handleTriggerThresholdUpdate(Instruction instruction) {
	if (instruction.parameters.size() < 3) return;
	Trigger trigger = (Trigger)std::any_cast<int>(instruction.parameters[1]);
	if(trigger == Trigger::Left)
		m_settings.leftTriggerThreshold = (uint8_t)std::any_cast<int>(instruction.parameters[2]);
	else
		m_settings.rightTriggerThreshold = (uint8_t)std::any_cast<int>(instruction.parameters[2]);
}

bool UDP::isActive() {
	if (m_socket.is_open() &&
	   (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - m_lastUpdate) <= std::chrono::seconds(15))) {
		return true;
	}

	return false;
}

s_scePadSettings UDP::getSettings() {
	std::lock_guard<std::mutex> guard(m_settingsLock);
	return m_settings;
}

void UDP::setVibrationToUdpConfig(s_ScePadVibrationParam vibration) {
	std::lock_guard<std::mutex> guard(m_settingsLock);
	m_settings.rumbleFromEmulatedController = vibration;
}

UDP::UDP() : m_socket(m_ioContext) {
	try {
		m_socket.open(asio::ip::udp::v4());
		m_socket.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), 6969));

		if (m_socket.is_open()) {
			m_listenThread = std::thread(&UDP::listen, this);
			m_listenThread.detach();
			LOGI("[UDP] Started");
		}
		else {
			LOGE("[UDP] Failed to start");
		}
	}
	catch (...) {
		LOGE("[UDP] Failed to start");
	}

	m_settings.udpConfig = true;
}

UDP::~UDP() {
	m_threadRunning = false;
	asio::error_code ec;
	m_socket.close(ec);
	m_ioContext.stop();

	m_threadRunning = false;
	if (m_listenThread.joinable()) {
		m_listenThread.join();
	}
}
