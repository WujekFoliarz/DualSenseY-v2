#ifndef SCEPADSETTINGS_H
#define SCEPADSETTINGS_H

#include <duaLib.h>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <cmath>
#include <log.hpp>
#include <audioPassthrough.hpp>

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
	uint32_t handle = 0;

	float led[3] = { 0,0,0 };
	bool audioToLed = false;

	int brightness = 0;
	bool disablePlayerLed = false;

	bool audioPassthrough = false;
	int speakerVolume = 0;
	int micGain = 8;
	int audioPath = 3;
	float hapticIntensity = 1.0f;

	int emulatedController = (int)EmulatedController::NONE;
};

static void applySettings(uint32_t userId, s_scePadSettings settings, AudioPassthrough& audio) {
	if (settings.audioToLed) {
		float peak = audio.getCurrentCapturePeak();
		float max = 1.0;
		s_SceLightBar lightbar = { (uint8_t)scaleFloatToInt(peak,max), (uint8_t)scaleFloatToInt(peak,max), (uint8_t)scaleFloatToInt(peak,max) };
		scePadSetLightBar(settings.handle, &lightbar);
	}
	else {
		s_SceLightBar lightbar = { (uint8_t)scaleFloatToInt(settings.led[0], 1.0f), (uint8_t)scaleFloatToInt(settings.led[1], 1.0f), (uint8_t)scaleFloatToInt(settings.led[2],1.0f) };
		scePadSetLightBar(settings.handle, &lightbar);
	}

	scePadSetPlayerLedBrightness(settings.handle, settings.brightness);
	scePadSetPlayerLed(settings.handle, settings.disablePlayerLed ? false : true);
	scePadSetAudioOutPath(settings.handle, settings.audioPath);

	s_SceControllerType controllerType = {};
	scePadGetControllerType(settings.handle, &controllerType);

	s_ScePadVolumeGain volume = {};
	volume.speakerVolume = settings.speakerVolume * 9;
	volume.micGain = settings.micGain * 6;
	scePadSetVolumeGain(settings.handle, &volume);

	audio.setHapticIntensityByUserId(userId, settings.hapticIntensity);
}

#endif