#include "scePadSettings.hpp"
#include "led.hpp"
#include <algorithm>

static std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();

void applySettings(uint32_t index, s_scePadSettings settings, AudioPassthrough& audio) {
	auto now = std::chrono::steady_clock::now();
	auto sec = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
	float elapsed = std::chrono::duration<float>(now - startTime).count();

	float discoModeTime = fmod(elapsed * settings.discoModeSpeed, 1.0f);
	s_SceLightBar rainbowLightbar = {};
	getRainbowColor(discoModeTime, rainbowLightbar);

	float audioPeak = audio.getCurrentCapturePeak();
	uint8_t audioPeakUint8 = (uint8_t)scaleFloatToInt(audioPeak, 1.0);

	if (settings.audioToLed && !settings.discoMode) {		
		s_SceLightBar lightbar = { audioPeakUint8, audioPeakUint8, audioPeakUint8 };
		scePadSetLightBar(g_scePad[index], &lightbar);
	}
	else if (settings.audioToLed && settings.discoMode) {
		s_SceLightBar lightbar = { std::min(audioPeakUint8, rainbowLightbar.r), std::min(audioPeakUint8, rainbowLightbar.g), std::min(audioPeakUint8, rainbowLightbar.b) };
		scePadSetLightBar(g_scePad[index], &lightbar);
	}
	else if (settings.discoMode) {
		scePadSetLightBar(g_scePad[index], &rainbowLightbar);
	}
	else {
		s_SceLightBar lightbar = { (uint8_t)scaleFloatToInt(settings.led[0], 1.0f), (uint8_t)scaleFloatToInt(settings.led[1], 1.0f), (uint8_t)scaleFloatToInt(settings.led[2],1.0f) };
		scePadSetLightBar(g_scePad[index], &lightbar);
	}

	scePadSetPlayerLedBrightness(g_scePad[index], settings.brightness);
	scePadSetPlayerLed(g_scePad[index], settings.disablePlayerLed ? false : true);
	scePadSetAudioOutPath(g_scePad[index], settings.audioPath);

	s_SceControllerType controllerType = {};
	scePadGetControllerType(g_scePad[index], &controllerType);

	s_ScePadVolumeGain volume = {};
	volume.speakerVolume = settings.speakerVolume * 9;
	volume.micGain = settings.micGain * 6;
	scePadSetVolumeGain(g_scePad[index], &volume);

	audio.setHapticIntensityByUserId(index + 1, settings.hapticIntensity);

	settings.stockTriggerParam.triggerMask |= settings.isLeftUsingDsxTrigger ? 0 : SCE_PAD_TRIGGER_EFFECT_TRIGGER_MASK_L2;
	settings.stockTriggerParam.triggerMask |= settings.isRightUsingDsxTrigger ? 0 : SCE_PAD_TRIGGER_EFFECT_TRIGGER_MASK_R2;
	scePadSetTriggerEffect(g_scePad[index], &settings.stockTriggerParam);

	uint8_t triggerBitmask = 0;
	triggerBitmask |= settings.isLeftUsingDsxTrigger ? SCE_PAD_TRIGGER_EFFECT_TRIGGER_MASK_L2 : 0;
	triggerBitmask |= settings.isRightUsingDsxTrigger ? SCE_PAD_TRIGGER_EFFECT_TRIGGER_MASK_R2 : 0;
	scePadSetTriggerEffectCustom(g_scePad[index], settings.leftCustomTrigger, settings.rightCustomTrigger, triggerBitmask);
}