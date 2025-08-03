#ifndef SCEPADSETTINGS_H
#define SCEPADSETTINGS_H


#include "scePadCustomTriggers.hpp"
#include "scePadHandle.hpp"

#include <duaLib.h>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <cmath>
#include <log.hpp>
#include <audioPassthrough.hpp>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <atomic>

#define TRIGGER_COUNT 2
#define SONY_FORMAT 0
#define DSX_FORMAT 1
#define L2 0
#define R2 1

constexpr int MAX_PARAM_COUNT = 11;

namespace TriggerStringSony {
	constexpr const char* OFF = "Off";
	constexpr const char* FEEDBACK = "Feedback";
	constexpr const char* WEAPON = "Weapon";
	constexpr const char* VIBRATION = "Vibration";
	constexpr const char* SLOPE_FEEDBACK = "Slope Feedback";
	constexpr const char* MULTIPLE_POSITION_FEEDBACK = "Multiple Position Feedback";
	constexpr const char* MULTIPLE_POSITION_VIBRATION = "Multiple Position Vibration";
}

namespace TriggerStringDSX {
	constexpr const char* Off = "Off";
	constexpr const char* Rigid = "Rigid";
	constexpr const char* Pulse = "Pulse";
	constexpr const char* Rigid_A = "Rigid_A";
	constexpr const char* Rigid_B = "Rigid_B";
	constexpr const char* Rigid_AB = "Rigid_AB";
	constexpr const char* Pulse_A = "Pulse_A";
	constexpr const char* Pulse_B = "Pulse_B";
	constexpr const char* Pulse_AB = "Pulse_AB";
	constexpr const char* Calibration = "Calibration";
	constexpr const char* Normal = "Normal";
	constexpr const char* GameCube = "GameCube";
	constexpr const char* VerySoft = "VerySoft";
	constexpr const char* Soft = "Soft";
	constexpr const char* Medium = "Medium";
	constexpr const char* Hard = "Hard";
	constexpr const char* VeryHard = "VeryHard";
	constexpr const char* Hardest = "Hardest";
	constexpr const char* VibrateTrigger = "VibrateTrigger";
	constexpr const char* VibrateTriggerPulse = "VibrateTriggerPulse";
	constexpr const char* Choppy = "Choppy";
	constexpr const char* CustomTriggerValue = "CustomTriggerValue";
	constexpr const char* Resistance = "Resistance";
	constexpr const char* Bow = "Bow";
	constexpr const char* Galloping = "Galloping";
	constexpr const char* SemiAutomaticGun = "SemiAutomaticGun";
	constexpr const char* AutomaticGun = "AutomaticGun";
	constexpr const char* Machine = "Machine";
	constexpr const char* VIBRATE_TRIGGER_10Hz = "VIBRATE_TRIGGER_10Hz";
}

inline uint32_t scaleFloatToInt(float input_float, float max_float) {
	const uint32_t max_int = 255;

	if (input_float < 0.0f) {
		input_float = 0.0f;
	}
	else if (input_float > max_float) {
		input_float = max_float;
	}

	uint32_t scaled_int = static_cast<uint32_t>((input_float / max_float) * max_int);

	return scaled_int;
}

enum class EmulatedController {
	NONE,
	XBOX360,
	DUALSHOCK4
};

struct s_scePadSettings {
	// LED Settings
	float led[3] = { 0,0,0 };
	bool audioToLed = false;
	int brightness = 0;
	bool disablePlayerLed = false;
	bool discoMode = false;
	float discoModeSpeed = 0.1f;

	// Audio passthrough settings
	bool audioPassthrough = false;
	int speakerVolume = 0;
	int micGain = 8;
	int audioPath = 3;
	float hapticIntensity = 1.0f;

	// Adaptive trigger ui section
	int uiSelectedTrigger = L2;
	int uiParameters[TRIGGER_COUNT][11] = { {0,0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0,0} };
	int uiTriggerFormat[TRIGGER_COUNT] = { SONY_FORMAT, SONY_FORMAT };
	std::string uiSelectedSonyTriggerMode[TRIGGER_COUNT] = { TriggerStringSony::OFF, TriggerStringSony::OFF };
	std::string uiSelectedDSXTriggerMode[TRIGGER_COUNT] = { TriggerStringSony::OFF, TriggerStringSony::OFF };

	// For DSX trigger format
	bool isLeftUsingDsxTrigger = false;
	bool isRightUsingDsxTrigger = false;
	uint8_t leftCustomTrigger[11] = {};
	uint8_t rightCustomTrigger[11] = {};

	// For Sony trigger format
	ScePadTriggerEffectParam stockTriggerParam = {};

	// Emulation
	int emulatedController = (int)EmulatedController::NONE;
	uint8_t leftTriggerThreshold = 0;
	uint8_t rightTriggerThreshold = 0;
	s_ScePadVibrationParam rumbleFromEmulatedController = { 0,0 };
	bool useRumbleFromEmulatedController = true;
	s_SceLightBar lightbarFromEmulatedController = { 0,0,0 };
	bool useLightbarFromEmulatedController = true;

	// Keyboard and mouse stuff
	bool emulateAnalogWsad = false;

	// Analog sticks
	int leftStickDeadzone = 0;
	int rightStickDeadzone = 0;
};


using TriggerHandler = std::function<void(s_scePadSettings&, int&, std::vector<uint8_t>&)>;

const std::unordered_map<std::string, TriggerHandler> sonyTriggerHandlers = {
	{ TriggerStringSony::OFF, [&](s_scePadSettings& s, int& triggerIndex, const std::vector<uint8_t>&) {
		uint8_t index = s.uiSelectedTrigger == L2 ? SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2 : SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_R2;
		s.stockTriggerParam.command[triggerIndex].mode = SCE_PAD_TRIGGER_EFFECT_MODE_OFF;
	}},
	{ TriggerStringSony::FEEDBACK, [&](s_scePadSettings& s, int& triggerIndex, const std::vector<uint8_t>& p) {
		if (p.size() < 2) return;
		s.stockTriggerParam.command[triggerIndex].mode = SCE_PAD_TRIGGER_EFFECT_MODE_FEEDBACK;
		s.stockTriggerParam.command[triggerIndex].commandData.feedbackParam.position = p[0];
		s.stockTriggerParam.command[triggerIndex].commandData.feedbackParam.strength = p[1];
	}},
	{ TriggerStringSony::WEAPON, [&](s_scePadSettings& s, int& triggerIndex, const std::vector<uint8_t>& p) {
		if (p.size() < 3) return;
		s.stockTriggerParam.command[triggerIndex].mode = SCE_PAD_TRIGGER_EFFECT_MODE_WEAPON;
		s.stockTriggerParam.command[triggerIndex].commandData.weaponParam.startPosition = p[0];
		s.stockTriggerParam.command[triggerIndex].commandData.weaponParam.endPosition = p[1];
		s.stockTriggerParam.command[triggerIndex].commandData.weaponParam.strength = p[2];
	}},
	{ TriggerStringSony::VIBRATION, [&](s_scePadSettings& s, int& triggerIndex, const std::vector<uint8_t>& p) {
		if (p.size() < 3) return;
		s.stockTriggerParam.command[triggerIndex].mode = SCE_PAD_TRIGGER_EFFECT_MODE_VIBRATION;
		s.stockTriggerParam.command[triggerIndex].commandData.vibrationParam.position = p[0];
		s.stockTriggerParam.command[triggerIndex].commandData.vibrationParam.amplitude = p[1];
		s.stockTriggerParam.command[triggerIndex].commandData.vibrationParam.frequency = p[2];
	}},
	{ TriggerStringSony::SLOPE_FEEDBACK, [&](s_scePadSettings& s, int& triggerIndex, const std::vector<uint8_t>& p) {
		if (p.size() < 4) return;
		s.stockTriggerParam.command[triggerIndex].mode = SCE_PAD_TRIGGER_EFFECT_MODE_SLOPE_FEEDBACK;
		s.stockTriggerParam.command[triggerIndex].commandData.slopeFeedbackParam.startPosition = p[0];
		s.stockTriggerParam.command[triggerIndex].commandData.slopeFeedbackParam.endPosition = p[1];
		s.stockTriggerParam.command[triggerIndex].commandData.slopeFeedbackParam.startStrength = p[2];
		s.stockTriggerParam.command[triggerIndex].commandData.slopeFeedbackParam.endStrength = p[3];
	}},
	{ TriggerStringSony::MULTIPLE_POSITION_FEEDBACK, [&](s_scePadSettings& s, int& triggerIndex, const std::vector<uint8_t>& p) {
		if (p.size() < 10) return;
		s.stockTriggerParam.command[triggerIndex].mode = SCE_PAD_TRIGGER_EFFECT_MODE_MULTIPLE_POSITION_FEEDBACK;
		for (int i = 0; i < 10; ++i)
			s.stockTriggerParam.command[triggerIndex].commandData.multiplePositionFeedbackParam.strength[i] = p[i];
	}},
	{ TriggerStringSony::MULTIPLE_POSITION_VIBRATION, [&](s_scePadSettings& s, int& triggerIndex, const std::vector<uint8_t>& p) {
		if (p.size() < 11) return;
		s.stockTriggerParam.command[triggerIndex].mode = SCE_PAD_TRIGGER_EFFECT_MODE_MULTIPLE_POSITION_VIBRATION;
		s.stockTriggerParam.command[triggerIndex].commandData.multiplePositionVibrationParam.frequency = p[0];
		for (int i = 1; i < 11; ++i)
			s.stockTriggerParam.command[triggerIndex].commandData.multiplePositionVibrationParam.amplitude[i - 1] = p[i];
	}}
};
const std::unordered_map<std::string, TriggerHandler> dsxTriggerHandlers = {
	{TriggerStringDSX::Normal, [&](s_scePadSettings& s, int& triggerIndex, const std::vector<uint8_t>& p) {customTriggerNormal(triggerIndex == L2 ? s.leftCustomTrigger : s.rightCustomTrigger);}},
	{TriggerStringDSX::GameCube, [&](s_scePadSettings& s, int& triggerIndex, const std::vector<uint8_t>& p) {customTriggerGamecube(triggerIndex == L2 ? s.leftCustomTrigger : s.rightCustomTrigger);}},
	{TriggerStringDSX::VerySoft, [&](s_scePadSettings& s, int& triggerIndex, const std::vector<uint8_t>& p) {customTriggerVerySoft(triggerIndex == L2 ? s.leftCustomTrigger : s.rightCustomTrigger);}},
	{TriggerStringDSX::Soft, [&](s_scePadSettings& s, int& triggerIndex, const std::vector<uint8_t>& p) {customTriggerSoft(triggerIndex == L2 ? s.leftCustomTrigger : s.rightCustomTrigger);}},
	{TriggerStringDSX::Medium, [&](s_scePadSettings& s, int& triggerIndex, const std::vector<uint8_t>& p) {customTriggerMedium(triggerIndex == L2 ? s.leftCustomTrigger : s.rightCustomTrigger);}},
	{TriggerStringDSX::Hard, [&](s_scePadSettings& s, int& triggerIndex, const std::vector<uint8_t>& p) {customTriggerHard(triggerIndex == L2 ? s.leftCustomTrigger : s.rightCustomTrigger);}},
	{TriggerStringDSX::VeryHard, [&](s_scePadSettings& s, int& triggerIndex, const std::vector<uint8_t>& p) {customTriggerVeryHard(triggerIndex == L2 ? s.leftCustomTrigger : s.rightCustomTrigger);}},
	{TriggerStringDSX::Hardest, [&](s_scePadSettings& s, int& triggerIndex, const std::vector<uint8_t>& p) {customTriggerHardest(triggerIndex == L2 ? s.leftCustomTrigger : s.rightCustomTrigger);}},
	{TriggerStringDSX::VibrateTrigger, [&](s_scePadSettings& s, int& triggerIndex, const std::vector<uint8_t>& p) {customTriggerVibrateTrigger(triggerIndex == L2 ? s.leftCustomTrigger : s.rightCustomTrigger);}},
	{TriggerStringDSX::VibrateTriggerPulse, [&](s_scePadSettings& s, int& triggerIndex, const std::vector<uint8_t>& p) {customTriggerVibrateTriggerPulse(triggerIndex == L2 ? s.leftCustomTrigger : s.rightCustomTrigger);}},
	{TriggerStringDSX::Choppy, [&](s_scePadSettings& s, int& triggerIndex, const std::vector<uint8_t>& p) {customTriggerChoppy(triggerIndex == L2 ? s.leftCustomTrigger : s.rightCustomTrigger);}},
	{TriggerStringDSX::CustomTriggerValue, [&](s_scePadSettings& s, int& triggerIndex, const std::vector<uint8_t>& p) {customTriggerCustomTriggerValue(p, triggerIndex == L2 ? s.leftCustomTrigger : s.rightCustomTrigger);}},
	{TriggerStringDSX::Resistance, [&](s_scePadSettings& s, int& triggerIndex, const std::vector<uint8_t>& p) {customTriggerResistance(p, triggerIndex == L2 ? s.leftCustomTrigger : s.rightCustomTrigger);}},
	{TriggerStringDSX::Bow, [&](s_scePadSettings& s, int& triggerIndex, const std::vector<uint8_t>& p) {customTriggerBow(p, triggerIndex == L2 ? s.leftCustomTrigger : s.rightCustomTrigger);}},
	{TriggerStringDSX::Galloping, [&](s_scePadSettings& s, int& triggerIndex, const std::vector<uint8_t>& p) {customTriggerGalloping(p, triggerIndex == L2 ? s.leftCustomTrigger : s.rightCustomTrigger);}},
	{TriggerStringDSX::SemiAutomaticGun, [&](s_scePadSettings& s, int& triggerIndex, const std::vector<uint8_t>& p) {customTriggerSemiAutomaticGun(p, triggerIndex == L2 ? s.leftCustomTrigger : s.rightCustomTrigger);}},
	{TriggerStringDSX::AutomaticGun, [&](s_scePadSettings& s, int& triggerIndex, const std::vector<uint8_t>& p) {customTriggerAutomaticGun(p, triggerIndex == L2 ? s.leftCustomTrigger : s.rightCustomTrigger);}},
	{TriggerStringDSX::Machine, [&](s_scePadSettings& s, int& triggerIndex, const std::vector<uint8_t>& p) {customTriggerMachine(p, triggerIndex == L2 ? s.leftCustomTrigger : s.rightCustomTrigger);}},
	{TriggerStringDSX::VIBRATE_TRIGGER_10Hz, [&](s_scePadSettings& s, int& triggerIndex, const std::vector<uint8_t>& p) {customTriggerVIBRATE_TRIGGER_10Hz(p, triggerIndex == L2 ? s.leftCustomTrigger : s.rightCustomTrigger);}},
};

void applySettings(uint32_t index, s_scePadSettings settings, AudioPassthrough& audio);

#endif