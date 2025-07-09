#ifndef AUDIOPASSTHROUGH_H
#define AUDIOPASSTHROUGH_H

#include <miniaudio.h>
#include <cstdint>
#include <vector>
#include <mutex>
#include <atomic>

class AudioPassthrough {
private:
	std::vector<float> m_audioBuffer[4];
	std::mutex m_bufferMutex;
	std::chrono::steady_clock::time_point lastTimeValidated;

	ma_device m_controller[4];
	ma_device_id m_controllerId[4];
	static ma_device m_captureDevice;
	bool m_active[4] = {false,false,false,false};
	uint32_t m_indexes[4] = { 0,1,2,3 };
	std::atomic<float> m_currentCapturePeak = 0.0f;
	std::atomic<float> m_hapticIntensity[4] = { 1.0f,1.0f,1.0f,1.0f };

	void startCaptureDevice(ma_device* pDevice);

	friend void captureDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
	friend void playbackDualshock4DataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
	friend void playbackDualsenseDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
public:	
	AudioPassthrough();
	~AudioPassthrough();

	void validate();
	bool startByUserId(uint32_t userId);
	bool stopByUserId(uint32_t userId);
	void setHapticIntensityByUserId(uint32_t userId, float intensity);
	float getCurrentCapturePeak();
};

#endif