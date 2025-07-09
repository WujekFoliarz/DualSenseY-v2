#include "audioPassthrough.hpp"
#include <duaLib.h>
#include "log.hpp"
#include <vector>
#include <mutex>
#include <chrono>
#include <cassert>
#include <iostream>
#include <string>
#include <stdexcept>
#include <codecvt>
#include <cmath>
#include <algorithm>

#if (!defined(__linux__)) && (!defined(__MACOS__))
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <guiddef.h>

static GUID StringToGuid(const std::string& guidStr) {
	GUID guid = {};

	unsigned int d1, d2, d3;
	unsigned int d4[8];

	int matched = std::sscanf(
		guidStr.c_str(),
		"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
		&d1, &d2, &d3,
		&d4[0], &d4[1], &d4[2], &d4[3], &d4[4], &d4[5], &d4[6], &d4[7]);

	guid.Data1 = d1;
	guid.Data2 = static_cast<WORD>(d2);
	guid.Data3 = static_cast<WORD>(d3);
	for (int i = 0; i < 8; ++i) {
		guid.Data4[i] = static_cast<BYTE>(d4[i]);
	}

	return guid;
}


static ma_result FindDeviceByContainerIdWindows(ma_context* pContext, const GUID& targetContainerId, ma_device_id* pDeviceId) {
	ma_device_info* pPlaybackInfos;
	ma_uint32 playbackCount;
	ma_result result = ma_context_get_devices(pContext, &pPlaybackInfos, &playbackCount, nullptr, nullptr);
	if (result != MA_SUCCESS) {
		printf("Failed to enumerate devices.\n");
		return result;
	}

	HRESULT hr;
	IMMDeviceEnumerator* pEnumerator = nullptr;
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
	if (FAILED(hr)) {
		printf("Failed to create device enumerator.\n");
		return MA_ERROR;
	}

	for (ma_uint32 i = 0; i < playbackCount; i++) {
		std::wstring wasapiId(pPlaybackInfos[i].id.wasapi);
		IMMDevice* pDevice = nullptr;
		hr = pEnumerator->GetDevice(wasapiId.c_str(), &pDevice);
		if (SUCCEEDED(hr)) {
			IPropertyStore* pProps = nullptr;
			hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
			if (SUCCEEDED(hr)) {
				PROPVARIANT pv;
				PropVariantInit(&pv);
				hr = pProps->GetValue(PKEY_Device_ContainerId, &pv);
				if (SUCCEEDED(hr) && pv.vt == VT_CLSID) {
					GUID containerId = *pv.puuid;
					if (containerId == targetContainerId) {
						*pDeviceId = pPlaybackInfos[i].id;
						PropVariantClear(&pv);
						pProps->Release();
						pDevice->Release();
						pEnumerator->Release();
						return MA_SUCCESS;
					}
				}
				PropVariantClear(&pv);
				pProps->Release();
			}
			pDevice->Release();
		}
	}

	pEnumerator->Release();
	return ma_result::MA_NO_DEVICE;
}
#endif

bool isMaDeviceWorking(ma_device* device) {
	if (device == nullptr) return false;

	ma_device_state state = ma_device_get_state(device);
	if (state == ma_device_state_uninitialized) return false;
	if (state == ma_device_state_starting) return false;
	if (state == ma_device_state_stopped) return false;
	if (state != ma_device_state_started) return false;
	if (!ma_device_is_started(device)) return false;

	return true;
}

ma_device AudioPassthrough::m_captureDevice;
ma_context g_context;
std::mutex g_contextLock;

void captureDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
	using namespace std;
	const float* inputF32 = static_cast<const float*>(pInput);
	const uint32_t numChannels = 2;  // Assuming stereo
	AudioPassthrough* userData = (AudioPassthrough*)pDevice->pUserData;
	float localPeak = 0.0f;

	std::lock_guard<std::mutex> lock(userData->m_bufferMutex);

	for (uint32_t frame = 0; frame < frameCount; ++frame) {
		float sampleL = inputF32[frame * numChannels + 0];
		float sampleR = inputF32[frame * numChannels + 1];

		for (uint32_t i = 0; i < 4; ++i) {
			userData->m_audioBuffer[i].push_back(sampleL);
			userData->m_audioBuffer[i].push_back(sampleR);

			if (userData->m_audioBuffer[i].size() > 48000 * numChannels) {
				userData->m_audioBuffer[i].clear();
			}
		}

		float absL = fabsf(sampleL);
		float absR = fabsf(sampleR);
		float maxSample = max(absL, absR);
		if (maxSample > localPeak) {
			localPeak = maxSample;
		}
	}

	userData->m_currentCapturePeak = localPeak;
}

void playbackDualsenseDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    float* out = (float*)pOutput;
	AudioPassthrough* userData = (AudioPassthrough*)pDevice->pUserData;
    std::lock_guard<std::mutex> lock(userData->m_bufferMutex);

	uint32_t index = 0;
	for (uint32_t i = 0; i < 4; i++) {
		if (&userData->m_controller[i] == pDevice) {
			index = i;
			break;
		}
	}

    size_t availableFrames = userData->m_audioBuffer[index].size() / 2;
    size_t framesToWrite = std::min<size_t>(frameCount, availableFrames);

    for (size_t i = 0; i < framesToWrite; ++i) {
        float inL = userData->m_audioBuffer[index][i * 2 + 0];
        float inR = userData->m_audioBuffer[index][i * 2 + 1];

        out[i * 4 + 0] = 0.0f;
        out[i * 4 + 1] = inL;     
        out[i * 4 + 2] = inL * userData->m_hapticIntensity[index];    
        out[i * 4 + 3] = inR * userData->m_hapticIntensity[index];
    }

    for (size_t i = framesToWrite; i < frameCount; ++i) {
        out[i * 4 + 0] = 0.0f;
        out[i * 4 + 1] = 0.0f;
        out[i * 4 + 2] = 0.0f;
        out[i * 4 + 3] = 0.0f;
    }

	userData->m_audioBuffer[index].erase(userData->m_audioBuffer[index].begin(), userData->m_audioBuffer[index].begin() + framesToWrite * 2);
}

void playbackDualshock4DataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
	float* out = (float*)pOutput;
	AudioPassthrough* userData = (AudioPassthrough*)pDevice->pUserData;
	std::lock_guard<std::mutex> lock(userData->m_bufferMutex);

	uint32_t index = 0;
	for (uint32_t i = 0; i < 4; i++) {
		if (&userData->m_controller[i] == pDevice) {
			index = i;
		}
	}

	size_t availableFrames = userData->m_audioBuffer[index].size() / 2;
	size_t framesToWrite = std::min<size_t>(frameCount, availableFrames);

	for (size_t i = 0; i < framesToWrite; ++i) {
		float inL = userData->m_audioBuffer[index][i * 2 + 0];
		float inR = userData->m_audioBuffer[index][i * 2 + 1];

		out[i * 2 + 0] = 0.0f;     
		out[i * 2 + 1] = inL;      
	}

	for (size_t i = framesToWrite; i < frameCount; ++i) {
		out[i * 2 + 0] = 0.0f;
		out[i * 2 + 1] = 0.0f;
	}

	userData->m_audioBuffer[index].erase(userData->m_audioBuffer[index].begin(), userData->m_audioBuffer[index].begin() + framesToWrite * 2);
}

void AudioPassthrough::startCaptureDevice(ma_device* pDevice) {
	if (isMaDeviceWorking(pDevice)) return;

	ma_device_config captureConfig = ma_device_config_init(ma_device_type_loopback);
	captureConfig.capture.format = ma_format_f32;
	captureConfig.capture.channels = 2;
	captureConfig.sampleRate = 48000;
	captureConfig.dataCallback = captureDataCallback;
	captureConfig.periodSizeInFrames = 128;
	captureConfig.periods = 2;
	captureConfig.pUserData = this;

	ma_result result = ma_device_init(NULL, &captureConfig, pDevice);
	if (result != MA_SUCCESS) {
		LOGE("[Audio Passthrough] Failed to init the capture device");
		return;
	}

	ma_device_start(pDevice);
}

AudioPassthrough::AudioPassthrough() {
	if (ma_context_init(NULL, 0, NULL, &g_context) != MA_SUCCESS) return;
	lastTimeValidated = std::chrono::steady_clock::now();
}

AudioPassthrough::~AudioPassthrough() {
	ma_device_uninit(&m_captureDevice);

	for (uint32_t i = 0; i < 4; i++) {
		if(isMaDeviceWorking(&m_controller[i]))
			ma_device_uninit(&m_controller[i]);
	}

	std::lock_guard<std::mutex> lock(m_bufferMutex);
	ma_context_uninit(&g_context);
}

void AudioPassthrough::validate() {
	auto now = std::chrono::steady_clock::now();
	auto timeSinceLastRetry = std::chrono::duration_cast<std::chrono::seconds>(now - lastTimeValidated);

	if (timeSinceLastRetry < std::chrono::seconds(5)) 
		return;

	if (!isMaDeviceWorking(&m_captureDevice)) {
		ma_device_uninit(&m_captureDevice);

		{
			std::lock_guard<std::mutex> lock(m_bufferMutex);
			for (uint32_t i = 0; i < 4; i++) {
				m_audioBuffer[i].clear();
				m_audioBuffer[i].shrink_to_fit();
			}	
		}

		startCaptureDevice(&m_captureDevice);
	}

	for (uint32_t i = 0; i < 4; i++) {
		if (m_active[i] && !isMaDeviceWorking(&m_controller[i])) {
			startByUserId(i + 1);
		}
	}

	lastTimeValidated = std::chrono::steady_clock::now();
}

bool AudioPassthrough::startByUserId(uint32_t userId) {
#if (!defined(__linux__)) && (!defined(__MACOS__))
	assert(userId >= 1 && userId <= 4);

	uint32_t index = userId - 1;

	if (m_active[index] && isMaDeviceWorking(&m_controller[index])) { 
		return false; 
	}
	else if (m_active[index] && !isMaDeviceWorking(&m_controller[index])) {
		ma_device_uninit(&m_controller[index]);
	}
	
	uint32_t handle = scePadGetHandle(userId, 0, 0);
	
	{
		std::lock_guard<std::mutex> lock(m_bufferMutex);
		for (uint32_t i = 0; i < 4; i++) {
			m_audioBuffer[i].clear();
		}
	}

    s_ScePadContainerIdInfo info = {};
	uint32_t result = scePadGetContainerIdInformation(handle, &info);

	if (result != SCE_OK) {
		return false;
	}

	GUID targetContainerId = StringToGuid(info.id);

	if (strlen(info.id) == 0) return false;

	{
		std::lock_guard<std::mutex> guard(g_contextLock);
		result = FindDeviceByContainerIdWindows(&g_context, targetContainerId, &m_controllerId[index]);
		if (result != MA_SUCCESS) return false;
	}

	s_SceControllerType controllerType = {};
	scePadGetControllerType(handle, &controllerType);

    ma_device_config playbackConfig = ma_device_config_init(ma_device_type_playback);
    playbackConfig.playback.format = ma_format_f32;
    playbackConfig.playback.channels = 0;
    playbackConfig.sampleRate = 48000;
    playbackConfig.dataCallback = controllerType == s_SceControllerType::DUALSENSE ? playbackDualsenseDataCallback : playbackDualshock4DataCallback;
	playbackConfig.playback.pDeviceID = &m_controllerId[index];
	playbackConfig.periodSizeInFrames = 128;
	playbackConfig.periods = 2;
	playbackConfig.pUserData = this;

    result = ma_device_init(NULL, &playbackConfig, &m_controller[index]);
    if (result != MA_SUCCESS) {
		LOGE("Failed to open playback device.");
        ma_device_uninit(&m_controller[index]);
        return false;
    }

	if (ma_device_start(&m_controller[index]) != MA_SUCCESS) {
		LOGE("Failed to start controller audio device.");
	}

	m_active[index] = true;
	return true;
#endif
	return false;
}

bool AudioPassthrough::stopByUserId(uint32_t userId) {
	assert(userId >= 1 && userId <= 4);

	uint32_t index = userId - 1;

	if (m_active[index]) {
		if (isMaDeviceWorking(&m_controller[index])) {
			ma_device_stop(&m_controller[index]);
		}
		ma_device_uninit(&m_controller[index]);

		{
			std::lock_guard<std::mutex> lock(m_bufferMutex);
			m_audioBuffer[index].clear();
			m_audioBuffer[index].shrink_to_fit();
		}
	}

	m_active[index] = false;

	return true;
}

void AudioPassthrough::setHapticIntensityByUserId(uint32_t userId, float intensity) {
	assert(userId >= 1 && userId <= 4);

	m_hapticIntensity[userId - 1] = intensity;
}

float AudioPassthrough::getCurrentCapturePeak() {
	return m_currentCapturePeak;
}
