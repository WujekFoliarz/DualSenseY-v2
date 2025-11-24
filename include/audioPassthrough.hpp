#ifndef AUDIOPASSTHROUGH_H
#define AUDIOPASSTHROUGH_H

#include <miniaudio.h>
#include <cstdint>
#include <vector>
#include <mutex>
#include <atomic>
#include <string>

class AudioPassthrough {
private:
	std::vector<float> m_AudioBuffer[4];
	std::vector<int8_t> m_DualsenseBtBuffer[4];
	std::mutex m_BufferMutex;
	std::chrono::steady_clock::time_point m_LastTimeValidated;

	ma_device m_Controller[4];
	ma_device_id m_ControllerId[4];
	static ma_device m_CaptureDevice;
	static ma_device m_CaptureDevice3000HzU8;
	bool m_Active[4] = {false,false,false,false};
	uint32_t m_Indexes[4] = { 0,1,2,3 };
	std::atomic<float> m_CurrentCapturePeak = 0.0f;
	std::atomic<float> m_HapticIntensity[4] = { 1.0f,1.0f,1.0f,1.0f };
	uint32_t m_CurrentCaptureDevice = 0;
	uint32_t m_LastCaptureDevice = 0;

	void StartCaptureDevice(ma_device* pDevice, ma_device_config* pConfig);
	void HapticTimerThread();

	friend void CaptureDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
	friend void PlaybackDualshock4DataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
	friend void PlaybackDualsenseDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
public:	
	AudioPassthrough();
	~AudioPassthrough();

	void Validate();
	bool StartByUserId(uint32_t userId);
	bool StopByUserId(uint32_t userId);
	void SetHapticIntensityByUserId(uint32_t userId, float intensity);
	float GetCurrentCapturePeak();
	void SetCaptureDevice(uint32_t Device = 0);
	std::vector<std::string> GetCaptureDeviceList();
};

#endif