#ifndef AUDIOPASSTHROUGH_H
#define AUDIOPASSTHROUGH_H

#include <miniaudio.h>
#include <cstdint>
#include <vector>
#include <mutex>
#include <atomic>
#include <string>
#include <cmath>

struct BiquadFilter {
	float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
	float a1 = 0.0f, a2 = 0.0f;
	float x1 = 0.0f, x2 = 0.0f;
	float y1 = 0.0f, y2 = 0.0f;

	void initLowPass(float cutoffHz, float sampleRate, float q = 0.70710678f) {
		float omega = 2.0f * 3.14159265358979323846f * cutoffHz / sampleRate;
		float sn = sinf(omega);
		float cs = cosf(omega);
		float alpha = sn / (2.0f * q);

		float a0 = 1.0f + alpha;
		b0 = ((1.0f - cs) * 0.5f) / a0;
		b1 = (1.0f - cs) / a0;
		b2 = ((1.0f - cs) * 0.5f) / a0;
		a1 = (-2.0f * cs) / a0;
		a2 = (1.0f - alpha) / a0;
		reset();
	}

	void reset() {
		x1 = 0.0f;
		x2 = 0.0f;
		y1 = 0.0f;
		y2 = 0.0f;
	}

	inline float process(float in) {
		float out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
		x2 = x1;
		x1 = in;
		y2 = y1;
		y1 = out;
		return out;
	}
};

struct HapticLowPassFilter {
	BiquadFilter stage1;
	BiquadFilter stage2;

	HapticLowPassFilter() {
		init(160.0f, 48000.0f);
	}

	void init(float cutoffHz, float sampleRate) {
		// 4th order Butterworth: Q1 = 0.54119610, Q2 = 1.30656296
		stage1.initLowPass(cutoffHz, sampleRate, 0.54119610f);
		stage2.initLowPass(cutoffHz, sampleRate, 1.30656296f);
	}

	void reset() {
		stage1.reset();
		stage2.reset();
	}

	inline float process(float in) {
		return stage2.process(stage1.process(in));
	}
};

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
	std::atomic<float> m_SpeakerVolume[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	HapticLowPassFilter m_HapticFilterL[4];
	HapticLowPassFilter m_HapticFilterR[4];
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
	void SetSpeakerVolumeByUserId(uint32_t userId, float volume);
	float GetCurrentCapturePeak();
	void SetCaptureDevice(uint32_t Device = 0);
	std::vector<std::string> GetCaptureDeviceList();
};

#endif