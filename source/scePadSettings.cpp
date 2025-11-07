#include "scePadSettings.hpp"
#include "led.hpp"
#include <algorithm>
#include <fstream>
#include <imgui.h>
#include <platform_folders.h>

static std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
// <Mac address, was loaded>
static std::unordered_map<std::string, bool> loadList;

void SaveSettingsToFile(const s_scePadSettings& s, const std::string& filepath) {
	nlohmann::json j = s;
	std::ofstream(filepath) << j.dump(4);
}

bool LoadSettingsFromFile(s_scePadSettings* s, const std::string& filepath) {
	try {
		std::ifstream ifs(filepath);
		if (!ifs) return false;
		nlohmann::json j;
		ifs >> j;
		*s = j.get<s_scePadSettings>();
		return true;
	}
	catch (...) {
		return false;
	}
}

bool GetDefaultConfigFromMac(const std::string& mac, s_scePadSettings* s) {
	std::string cleanMac = mac;
	cleanMac.erase(std::remove(cleanMac.begin(), cleanMac.end(), ':'), cleanMac.end());
	std::filesystem::path filePath = std::filesystem::path(sago::getDocumentsFolder() + "/DSY/DefaultConfigs/" + cleanMac);
	
	if (std::filesystem::exists(filePath)) {
		std::ifstream file(filePath);
		std::string configPath = "";
		file >> configPath;
		file.close();
		LoadSettingsFromFile(s, configPath);
		return true;
	}

	return false;
}

bool RemoveDefaultConfigByMac(const std::string& mac) {
	std::string cleanMac = mac;
	cleanMac.erase(std::remove(cleanMac.begin(), cleanMac.end(), ':'), cleanMac.end());
	std::filesystem::path filePath = std::filesystem::path(sago::getDocumentsFolder() + "/DSY/DefaultConfigs/" + cleanMac);

	if (std::filesystem::exists(filePath)) {
		std::filesystem::remove(filePath);
		return true;
	}

	return false;
}



void LoadDefaultConfig(int currentController, s_scePadSettings* s) {
	std::string macAddress = scePadGetMacAddress(g_ScePad[currentController]);

	if (macAddress != "" && !loadList[macAddress]) {
		if (GetDefaultConfigFromMac(macAddress, s)) {
			loadList[macAddress] = true;
		}
	}
}

void ForceControllerToNotLoadDefault(int controller) {
	std::string macAddress = scePadGetMacAddress(g_ScePad[controller]);
	loadList[macAddress] = true;
}

void applySettings(uint32_t index, s_scePadSettings settings, AudioPassthrough& audio) {
	auto now = std::chrono::steady_clock::now();
	auto sec = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
	float elapsed = std::chrono::duration<float>(now - startTime).count();

	float discoModeTime = fmod(elapsed * settings.discoModeSpeed, 1.0f);
	s_SceLightBar rainbowLightbar = {};
	GetRainbowColor(discoModeTime, rainbowLightbar);

	float audioPeak = audio.GetCurrentCapturePeak();
	uint8_t audioPeakUint8 = (uint8_t)scaleFloatToInt(audioPeak, 1.0);

	if (settings.useLightbarFromEmulatedController && (settings.emulatedController == (int)EmulatedController::DUALSHOCK4 || settings.usingPeerController)) {
		scePadSetLightBar(g_ScePad[index], &settings.lightbarFromEmulatedController);
	}
	else if (settings.audioToLed && !settings.discoMode) {
		s_SceLightBar lightbar = { audioPeakUint8, audioPeakUint8, audioPeakUint8 };
		scePadSetLightBar(g_ScePad[index], &lightbar);
	}
	else if (settings.audioToLed && settings.discoMode) {
		s_SceLightBar lightbar = { std::min(audioPeakUint8, rainbowLightbar.r), std::min(audioPeakUint8, rainbowLightbar.g), std::min(audioPeakUint8, rainbowLightbar.b) };
		scePadSetLightBar(g_ScePad[index], &lightbar);
	}
	else if (settings.discoMode) {
		scePadSetLightBar(g_ScePad[index], &rainbowLightbar);
	}
	else {
		s_SceLightBar lightbar = { (uint8_t)scaleFloatToInt(settings.led[0], 1.0f), (uint8_t)scaleFloatToInt(settings.led[1], 1.0f), (uint8_t)scaleFloatToInt(settings.led[2],1.0f) };
		scePadSetLightBar(g_ScePad[index], &lightbar);
	}

	scePadSetPlayerLedBrightness(g_ScePad[index], settings.brightness);
	scePadSetPlayerLed(g_ScePad[index], settings.disablePlayerLed ? false : true);
	scePadSetAudioOutPath(g_ScePad[index], settings.audioPath);

	s_SceControllerType controllerType = {};
	scePadGetControllerType(g_ScePad[index], &controllerType);

	s_ScePadVolumeGain volume = {};
	volume.speakerVolume = settings.speakerVolume * 9;
	volume.micGain = settings.micGain * 6;
	scePadSetVolumeGain(g_ScePad[index], &volume);

	audio.SetHapticIntensityByUserId(index + 1, settings.hapticIntensity);

	int l2Value = settings.rumbleToAt_swapTriggers ? settings.rumbleFromEmulatedController.smallMotor : settings.rumbleFromEmulatedController.largeMotor;
	int r2Value = settings.rumbleToAt_swapTriggers ? settings.rumbleFromEmulatedController.largeMotor : settings.rumbleFromEmulatedController.smallMotor;
	
	if (settings.triggersAsButtons && (settings.usingPeerController || settings.emulatedController != (int)EmulatedController::NONE)) {
		uint8_t leftTrigger[11] = {};
		uint8_t rightTrigger[11] = {};

		CustomTriggerHardestB(leftTrigger,settings.triggersAsButtonStartPos);
		CustomTriggerHardestB(rightTrigger, settings.triggersAsButtonStartPos);
		scePadSetTriggerEffectCustom(g_ScePad[index], leftTrigger, rightTrigger, SCE_PAD_TRIGGER_EFFECT_TRIGGER_MASK_L2 | SCE_PAD_TRIGGER_EFFECT_TRIGGER_MASK_R2);
	}
	else if (settings.rumbleToAT && (settings.usingPeerController || settings.emulatedController != (int)EmulatedController::NONE )) {
		uint8_t leftTrigger[11] = {};
		uint8_t rightTrigger[11] = {};

		std::vector<uint8_t> l2Param;
		l2Param.push_back(std::min(l2Value, settings.rumbleToAt_frequency[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2]));
		l2Param.push_back(std::min(l2Value, settings.rumbleToAt_intensity[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2]));
		l2Param.push_back(settings.rumbleToAt_position[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2]);

		std::vector<uint8_t> r2Param;
		r2Param.push_back(std::min(r2Value, settings.rumbleToAt_frequency[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_R2]));
		r2Param.push_back(std::min(r2Value, settings.rumbleToAt_intensity[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_R2]));
		r2Param.push_back(settings.rumbleToAt_position[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_R2]);

		CustomTriggerBetterVibration(l2Param, leftTrigger);
		CustomTriggerBetterVibration(r2Param, rightTrigger);

		scePadSetTriggerEffectCustom(g_ScePad[index], leftTrigger, rightTrigger, SCE_PAD_TRIGGER_EFFECT_TRIGGER_MASK_L2 | SCE_PAD_TRIGGER_EFFECT_TRIGGER_MASK_R2);
	}
	else {
		settings.stockTriggerParam.triggerMask |= settings.isLeftUsingDsxTrigger ? 0 : SCE_PAD_TRIGGER_EFFECT_TRIGGER_MASK_L2;
		settings.stockTriggerParam.triggerMask |= settings.isRightUsingDsxTrigger ? 0 : SCE_PAD_TRIGGER_EFFECT_TRIGGER_MASK_R2;
		scePadSetTriggerEffect(g_ScePad[index], &settings.stockTriggerParam);

		uint8_t triggerBitmask = 0;
		triggerBitmask |= settings.isLeftUsingDsxTrigger ? SCE_PAD_TRIGGER_EFFECT_TRIGGER_MASK_L2 : 0;
		triggerBitmask |= settings.isRightUsingDsxTrigger ? SCE_PAD_TRIGGER_EFFECT_TRIGGER_MASK_R2 : 0;
		scePadSetTriggerEffectCustom(g_ScePad[index], settings.leftCustomTrigger.data(), settings.rightCustomTrigger.data(), triggerBitmask);
	}

	if (settings.useRumbleFromEmulatedController || settings.udpConfig) {
		if (settings.rumbleFromEmulatedController.largeMotor > 0 || settings.rumbleFromEmulatedController.smallMotor > 0) {
			scePadSetVibrationMode(g_ScePad[index], SCE_PAD_RUMBLE_MODE);
		}
		else {
			scePadSetVibrationMode(g_ScePad[index], SCE_PAD_HAPTICS_MODE);
		}

		scePadSetVibration(g_ScePad[index], &settings.rumbleFromEmulatedController);
	}
}