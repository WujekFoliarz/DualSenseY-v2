#include "audioPassthrough.hpp"
#include <duaLib.h>
#include "log.hpp"
#include <vector>
#include <mutex>
#include <chrono>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <codecvt>
#include <cmath>
#include <algorithm>
#include <cstring>
#include "scePadHandle.hpp"
#include <thread>
#include <hidapi.h>

#ifdef WINDOWS
#include <Windows.h>
#include <timeapi.h>
#pragma comment(lib, "winmm.lib")
#endif

#if (!defined(__linux__)) && (!defined(__MACOS__))
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <guiddef.h>

static GUID StringToGuid(const std::string &guidStr)
{
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
	for (int i = 0; i < 8; ++i)
	{
		guid.Data4[i] = static_cast<BYTE>(d4[i]);
	}

	return guid;
}

static ma_result FindDeviceByContainerIdWindows(ma_context *pContext, const GUID &targetContainerId, ma_device_id *pDeviceId)
{
	ma_device_info *pPlaybackInfos;
	ma_uint32 playbackCount;
	ma_result result = ma_context_get_devices(pContext, &pPlaybackInfos, &playbackCount, nullptr, nullptr);
	if (result != MA_SUCCESS)
	{
		printf("Failed to enumerate devices.\n");
		return result;
	}

	HRESULT hr;
	IMMDeviceEnumerator *pEnumerator = nullptr;
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void **)&pEnumerator);
	if (FAILED(hr))
	{
		printf("Failed to create device enumerator.\n");
		return MA_ERROR;
	}

	for (ma_uint32 i = 0; i < playbackCount; i++)
	{
		std::wstring wasapiId(pPlaybackInfos[i].id.wasapi);
		IMMDevice *pDevice = nullptr;
		hr = pEnumerator->GetDevice(wasapiId.c_str(), &pDevice);
		if (SUCCEEDED(hr))
		{
			IPropertyStore *pProps = nullptr;
			hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
			if (SUCCEEDED(hr))
			{
				PROPVARIANT pv;
				PropVariantInit(&pv);
				hr = pProps->GetValue(PKEY_Device_ContainerId, &pv);
				if (SUCCEEDED(hr) && pv.vt == VT_CLSID)
				{
					GUID containerId = *pv.puuid;
					if (containerId == targetContainerId)
					{
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
#else
#include <libudev.h>
#include <regex>
#include <optional>

std::optional<std::string> ExtractUsbPort(const std::string &path)
{
	std::regex re(R"(/usb\d+/([^/]+))");
	std::smatch m;
	if (std::regex_search(path, m, re) && m.size() > 1)
	{
		std::string token = m[1].str();

		auto pos = token.find(':');
		if (pos != std::string::npos)
			token.resize(pos);
		return token;
	}
	return std::nullopt;
}

std::optional<std::string> ExtractVendorProduct(const std::string &path)
{
	std::regex re(R"(([0-9A-Fa-f]{4}:[0-9A-Fa-f]{4}))");
	std::smatch m;
	if (std::regex_search(path, m, re) && m.size() > 1)
	{
		return m[1].str();
	}
	return std::nullopt;
}

bool SamePhysicalDeviceByUsbPort(const std::string &a, const std::string &b)
{
	auto pa = ExtractUsbPort(a);
	auto pb = ExtractUsbPort(b);
	return pa && pb && (*pa == *pb);
}

bool SameDevice(const std::string &a, const std::string &b)
{
	if (SamePhysicalDeviceByUsbPort(a, b))
		return true;
	auto va = ExtractVendorProduct(a);
	auto vb = ExtractVendorProduct(b);
	return va && vb && (*va == *vb);
}

bool SerialMatch(const std::string &shortName, const std::string &longName)
{
	auto norm = [](std::string s)
	{
		for (char &c : s)
		{
			if (c == ' ')
				c = '_';
		}
		return s;
	};

	std::string a = norm(shortName);
	std::string b = norm(longName);

	return b.find(a) != std::string::npos;
}

std::string FindAudioDevPath(struct udev *udev_ctx, const std::string &pa_unique_id)
{
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *entry;
	std::string audio_devpath;

	enumerate = udev_enumerate_new(udev_ctx);
	if (!enumerate)
		return "";

	udev_enumerate_add_match_subsystem(enumerate, "sound");

	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);

	udev_list_entry_foreach(entry, devices)
	{
		const char *syspath = udev_list_entry_get_name(entry);
		struct udev_device *dev = udev_device_new_from_syspath(udev_ctx, syspath);

		if (dev)
		{
			struct udev_device *parent_usb = udev_device_get_parent_with_subsystem_devtype(
				dev, "usb", "usb_device");

			if (parent_usb)
			{
				const char *id_serial = udev_device_get_property_value(parent_usb, "ID_SERIAL");

				if (SameDevice(pa_unique_id, syspath))
				{
					audio_devpath = udev_device_get_devpath(dev);
					udev_device_unref(parent_usb);
					udev_enumerate_unref(enumerate);
					return std::string(id_serial);
				}
			}
		}
	}

	udev_enumerate_unref(enumerate);
	return "";
}

ma_result FindDeviceByContainerIdLinux(ma_context *pContext, const std::string &targetContainerId, ma_device_id *pDeviceId)
{
	ma_device_info *pPlaybackInfos;
	ma_uint32 playbackCount;
	if (ma_context_get_devices(pContext, &pPlaybackInfos, &playbackCount, nullptr, nullptr) != MA_SUCCESS)
	{
		return ma_result::MA_NO_DEVICE;
	}
	udev *udev_ctx = udev_new();
	std::string devpath = FindAudioDevPath(udev_ctx, targetContainerId);

	for (ma_uint32 iDevice = 0; iDevice < playbackCount; iDevice += 1)
	{
		if (SerialMatch(devpath, pPlaybackInfos[iDevice].id.pulse))
		{
			*pDeviceId = pPlaybackInfos[iDevice].id;
			udev_unref(udev_ctx);
			return ma_result::MA_SUCCESS;
		}
	}
	udev_unref(udev_ctx);
	return ma_result::MA_NO_DEVICE;
}

#endif

bool isMaDeviceWorking(ma_device *device)
{
	if (device == nullptr)
		return false;

	ma_device_state state = ma_device_get_state(device);
	if (state == ma_device_state_uninitialized)
		return false;
	if (state == ma_device_state_starting)
		return false;
	if (state == ma_device_state_stopped)
		return false;
	if (state != ma_device_state_started)
		return false;
	if (!ma_device_is_started(device))
		return false;

	return true;
}

ma_device AudioPassthrough::m_CaptureDevice;
ma_device AudioPassthrough::m_CaptureDevice3000HzU8;
ma_context g_context;
std::mutex g_contextLock;

void CaptureDataCallback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
	using namespace std;
	const float *inputF32 = static_cast<const float *>(pInput);
	const uint32_t numChannels = 2;
	AudioPassthrough *userData = (AudioPassthrough *)pDevice->pUserData;
	float localPeak = 0.0f;

	std::lock_guard<std::mutex> lock(userData->m_BufferMutex);
	for (uint32_t frame = 0; frame < frameCount; ++frame)
	{
		float sampleL = inputF32[frame * numChannels + 0];
		float sampleR = inputF32[frame * numChannels + 1];

		for (uint32_t i = 0; i < 4; ++i)
		{
			userData->m_AudioBuffer[i].push_back(sampleL);
			userData->m_AudioBuffer[i].push_back(sampleR);

			if (userData->m_AudioBuffer[i].size() > 48000 * numChannels)
			{
				userData->m_AudioBuffer[i].clear();
			}
		}

		float monoSampleF32 = (sampleL + sampleR) * 0.5f;

		float absL = fabsf(sampleL);
		float absR = fabsf(sampleR);
		float maxSample = max(absL, absR);
		if (maxSample > localPeak)
		{
			localPeak = maxSample;
		}
	}

	userData->m_CurrentCapturePeak = localPeak;
}

AudioPassthrough::~AudioPassthrough()
{
	ma_device_uninit(&m_CaptureDevice);

	for (uint32_t i = 0; i < 4; i++)
	{
		if (isMaDeviceWorking(&m_Controller[i]))
			ma_device_uninit(&m_Controller[i]);
	}

	std::lock_guard<std::mutex> lock(m_BufferMutex);
	ma_context_uninit(&g_context);
}

uint32_t crc32(const uint8_t *data, size_t size)
{
	uint32_t crc = ~0xEADA2D49; // 0xA2 seed

	while (size--)
	{
		crc ^= *data++;
		for (unsigned i = 0; i < 8; i++)
			crc = ((crc >> 1) ^ (0xEDB88320 & -(crc & 1)));
	}

	return ~crc;
}

// https://github.com/egormanga/SAxense
static constexpr uint32_t SAMPLE_RATE = 3000;
static constexpr uint32_t SAMPLE_SIZE = 64;
static constexpr uint32_t REPORT_SIZE = 141;
static constexpr uint32_t REPORT_ID = 0x32;
#pragma pack(push, 1)
struct packet
{
	uint8_t pid : 6;
	uint8_t unk : 1;
	uint8_t sized : 1;
	uint8_t length;
	uint8_t data[1];
} packet_t;

struct report
{
	uint8_t report_id;
	union
	{
		struct
		{
			uint8_t tag : 4;
			uint8_t seq : 4;
			uint8_t data[1];
		};
		struct
		{
			uint8_t payload[REPORT_SIZE - sizeof(uint32_t)];
			uint32_t crc;
		};
	};
};
#pragma pack(pop)

std::atomic<bool> g_running(true);
report *g_report = nullptr;
uint8_t *g_sample = nullptr;
uint8_t *g_ii = nullptr;
std::mutex g_bufferMutex;
constexpr size_t MAX_BUFFER_SIZE = SAMPLE_RATE / 6;
std::vector<uint8_t> g_audioBuffer(MAX_BUFFER_SIZE, 128);
size_t g_writePos = 0; 
size_t g_audioPos = 0;

bool initHapticReport()
{
	g_report = new report;
	memset(g_report, 0, sizeof(report));
	g_report->report_id = REPORT_ID;

	const size_t packet_0x11_size = 2 + 7;
	const size_t packet_0x12_size = 2 + SAMPLE_SIZE;
	uint8_t packet_0x11_data[9] = {0x11 | (1 << 7), 7, 0b11111110, 0, 0, 0, 0, 0xFF, 0};
	uint8_t packet_0x12_data[66] = {0x12 | (1 << 7), SAMPLE_SIZE};

	memcpy(g_report->data, packet_0x11_data, packet_0x11_size);
	memcpy(g_report->data + packet_0x11_size, packet_0x12_data, 2);

	g_ii = &g_report->data[2 + 6];				 
	g_sample = &g_report->data[packet_0x11_size + 2]; 

	return true;
}

void AudioPassthrough::HapticTimerThread() {
#ifdef WINDOWS 
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	timeBeginPeriod(1);

	EXECUTION_STATE prevState = SetThreadExecutionState(
		ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED
	);

	HANDLE hTimer = CreateWaitableTimerEx(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
	if (hTimer == NULL) {
		fprintf(stderr, "CreateWaitableTimerEx failed: %lu\n", GetLastError());
	}
#endif

	using namespace std::chrono;
	auto period_us = (1000000LL * SAMPLE_SIZE) / (SAMPLE_RATE * 2);
	auto period = microseconds(period_us);

	while (g_running) {
		auto start = steady_clock::now();

		uint8_t samples[4][SAMPLE_SIZE];
		{
			std::lock_guard<std::mutex> lock(g_bufferMutex);

			size_t bufSize = g_audioBuffer.size();
			if (bufSize == 0) {
				continue;
			}
			for (size_t i = 0; i < SAMPLE_SIZE; i++) {
				for (int j = 0; j < 4; j++) {
					int8_t sample = static_cast<int8_t>(g_audioBuffer[(g_audioPos + i) % bufSize] - 128);
					sample = static_cast<int8_t>(std::clamp<int>(sample * m_HapticIntensity[j], -128, 127));
					samples[j][i] = static_cast<uint8_t>(sample);
				}
			}
			g_audioPos = (g_audioPos + SAMPLE_SIZE) % bufSize;
		}

		(*g_ii)++;

		for (int i = 0; i < 4; i++) {

			int busType = -1;
			int result = scePadGetControllerBusType(g_ScePad[i], &busType);
			if (result != SCE_OK || busType != SCE_PAD_BUSTYPE_BT)
				continue;

			hid_device* device = (hid_device*)scePadGetHidApiHandle(g_ScePad[i]);
			if (device == nullptr || m_Active[i] == false)
				continue;

			for (int j = 0; j < SAMPLE_SIZE; j++) {
				g_sample[j] = samples[i][j];
			}
			g_report->crc = crc32(reinterpret_cast<uint8_t*>(g_report), 1 + sizeof(g_report->payload));
			int res = hid_write(device, reinterpret_cast<uint8_t*>(g_report), sizeof(report));

			if (res < 0) {
				fprintf(stderr, "Error writing to device: %ls\n", hid_error(device));
			}
		}

		auto elapsed = steady_clock::now() - start;
		auto sleepTime = duration_cast<microseconds>(period - elapsed);

	#ifdef WINDOWS
		if (sleepTime.count() > 0 && hTimer != NULL) {
			LARGE_INTEGER liDueTime;
			liDueTime.QuadPart = -(static_cast<LONGLONG>(sleepTime.count()) * 10LL);

			if (!SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, FALSE)) {
				fprintf(stderr, "SetWaitableTimer failed: %lu\n", GetLastError());
			}
			else {
				WaitForSingleObject(hTimer, INFINITE);
			}
		}
	#else
		if (sleepTime.count() > 0)
			std::this_thread::sleep_for(sleepTime);
	#endif
	}

#ifdef WINDOWS
	if (hTimer != NULL) {
		CloseHandle(hTimer);
	}
	SetThreadExecutionState(prevState);
	timeEndPeriod(1);
#endif
}

void PlaybackBTCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    std::lock_guard<std::mutex> lock(g_bufferMutex);
    const uint8_t* in = reinterpret_cast<const uint8_t*>(pInput);
    for (ma_uint32 i = 0; i < frameCount * 2; i++) {
        g_audioBuffer[g_writePos] = in[i];
        g_writePos = (g_writePos + 1) % MAX_BUFFER_SIZE;
    }
}

void AudioPassthrough::Validate()
{
	auto now = std::chrono::steady_clock::now();
	auto m_TimeSinceLastRetry = std::chrono::duration_cast<std::chrono::seconds>(now - m_LastTimeValidated);

	if (m_TimeSinceLastRetry < std::chrono::seconds(5))
		return;

	if (!isMaDeviceWorking(&m_CaptureDevice) || m_CurrentCaptureDevice != m_LastCaptureDevice)
	{
		ma_device_uninit(&m_CaptureDevice);

		{
			std::lock_guard<std::mutex> lock(m_BufferMutex);
			for (uint32_t i = 0; i < 4; i++)
			{
				m_AudioBuffer[i].clear();
				m_AudioBuffer[i].shrink_to_fit();
			}
		}

	#ifdef WINDOWS
		ma_device_config captureConfig = ma_device_config_init(ma_device_type_loopback);
	#else
		ma_device_config captureConfig = ma_device_config_init(ma_device_type_capture);
	#endif
		captureConfig.capture.format = ma_format_f32;
		captureConfig.capture.channels = 2;
		captureConfig.sampleRate = 48000;
		captureConfig.dataCallback = CaptureDataCallback;
		captureConfig.periodSizeInFrames = 128;
		captureConfig.periods = 2;
		captureConfig.pUserData = this;
		StartCaptureDevice(&m_CaptureDevice, &captureConfig);
	}

	if (!isMaDeviceWorking(&m_CaptureDevice3000HzU8) || m_CurrentCaptureDevice != m_LastCaptureDevice)
	{
		ma_device_uninit(&m_CaptureDevice3000HzU8);

	#ifdef WINDOWS
		ma_device_config captureConfig = ma_device_config_init(ma_device_type_loopback);
	#else
		ma_device_config captureConfig = ma_device_config_init(ma_device_type_capture);
	#endif
		captureConfig.capture.format = ma_format_u8;
		captureConfig.capture.channels = 2;
		captureConfig.sampleRate = SAMPLE_RATE;
		captureConfig.periodSizeInMilliseconds = (uint32_t)(1000.0 * (double)SAMPLE_SIZE / (double)(SAMPLE_RATE * 2));
		captureConfig.dataCallback = PlaybackBTCallback;
		captureConfig.periods = 1;
		captureConfig.pUserData = this;
		StartCaptureDevice(&m_CaptureDevice3000HzU8, &captureConfig);
	}

	for (uint32_t i = 0; i < 4; i++)
	{
		if (m_Active[i] && !isMaDeviceWorking(&m_Controller[i]))
		{
			StartByUserId(i + 1);
		}
	}

	m_LastTimeValidated = std::chrono::steady_clock::now();
	m_LastCaptureDevice = m_CurrentCaptureDevice;
}

void PlaybackDualsenseDataCallback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
	float *out = (float *)pOutput;
	AudioPassthrough *userData = (AudioPassthrough *)pDevice->pUserData;
	std::lock_guard<std::mutex> lock(userData->m_BufferMutex);

	uint32_t index = 0;
	for (uint32_t i = 0; i < 4; i++)
	{
		if (&userData->m_Controller[i] == pDevice)
		{
			index = i;
			break;
		}
	}

	size_t availableFrames = userData->m_AudioBuffer[index].size() / 2;
	size_t framesToWrite = std::min<size_t>(frameCount, availableFrames);

	for (size_t i = 0; i < framesToWrite; ++i)
	{
		float inL = userData->m_AudioBuffer[index][i * 2 + 0];
		float inR = userData->m_AudioBuffer[index][i * 2 + 1];

		out[i * 4 + 0] = 0.0f;
		out[i * 4 + 1] = std::clamp(inL, -1.0f, 1.0f);
		out[i * 4 + 2] = std::clamp(inL * userData->m_HapticIntensity[index], -1.0f, 1.0f);
		out[i * 4 + 3] = std::clamp(inR * userData->m_HapticIntensity[index], -1.0f, 1.0f);
	}

	for (size_t i = framesToWrite; i < frameCount; ++i)
	{
		out[i * 4 + 0] = 0.0f;
		out[i * 4 + 1] = 0.0f;
		out[i * 4 + 2] = 0.0f;
		out[i * 4 + 3] = 0.0f;
	}

	userData->m_AudioBuffer[index].erase(userData->m_AudioBuffer[index].begin(), userData->m_AudioBuffer[index].begin() + framesToWrite * 2);
}

void PlaybackDualshock4DataCallback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
	float *out = (float *)pOutput;
	AudioPassthrough *userData = (AudioPassthrough *)pDevice->pUserData;
	std::lock_guard<std::mutex> lock(userData->m_BufferMutex);

	uint32_t index = 0;
	for (uint32_t i = 0; i < 4; i++)
	{
		if (&userData->m_Controller[i] == pDevice)
		{
			index = i;
		}
	}

	size_t availableFrames = userData->m_AudioBuffer[index].size() / 2;
	size_t framesToWrite = std::min<size_t>(frameCount, availableFrames);

	for (size_t i = 0; i < framesToWrite; ++i)
	{
		float inL = userData->m_AudioBuffer[index][i * 2 + 0];
		float inR = userData->m_AudioBuffer[index][i * 2 + 1];

		out[i * 2 + 0] = 0.0f;
		out[i * 2 + 1] = std::clamp(inL, -1.0f, 1.0f);
	}

	for (size_t i = framesToWrite; i < frameCount; ++i)
	{
		out[i * 2 + 0] = 0.0f;
		out[i * 2 + 1] = 0.0f;
	}

	userData->m_AudioBuffer[index].erase(userData->m_AudioBuffer[index].begin(), userData->m_AudioBuffer[index].begin() + framesToWrite * 2);
}

void AudioPassthrough::StartCaptureDevice(ma_device *pDevice, ma_device_config *pConfig)
{
	if (isMaDeviceWorking(pDevice))
		return;
	if (!pConfig)
		return;

	ma_device_info *pCaptureInfos;
	ma_uint32 captureCount;
	ma_result result = ma_context_get_devices(&g_context, nullptr, nullptr, &pCaptureInfos, &captureCount);
	if (result != MA_SUCCESS)
	{
		printf("Failed to enumerate devices.\n");
		return;
	}

#ifdef LINUX
	ma_device_id deviceId = {};
	if (m_CurrentCaptureDevice >= 0 && m_CurrentCaptureDevice <= captureCount)
	{
		deviceId = pCaptureInfos[m_CurrentCaptureDevice].id;
	}
	else
	{
		deviceId = pCaptureInfos[0].id;
	}

	pConfig->capture.pDeviceID = &deviceId;
#endif;

	result = ma_device_init(&g_context, pConfig, pDevice);
	if (result != MA_SUCCESS)
	{
		LOGE("[Audio Passthrough] Failed to init the capture device");
		return;
	}

	ma_device_start(pDevice);
}

AudioPassthrough::AudioPassthrough()
{
	if (ma_context_init(NULL, 0, NULL, &g_context) != MA_SUCCESS)
		return;
	m_LastTimeValidated = std::chrono::steady_clock::now();
	initHapticReport();
	std::thread hapticThread(&AudioPassthrough::HapticTimerThread,this);
	hapticThread.detach();
}

bool AudioPassthrough::StartByUserId(uint32_t userId)
{
	assert(userId >= 1 && userId <= 4);

	uint32_t index = userId - 1;

	uint32_t handle = scePadGetHandle(userId, 0, 0);

	int busType = -1;
	uint32_t result = scePadGetControllerBusType(handle, &busType);
	if (result == SCE_OK && busType == SCE_PAD_BUSTYPE_BT) {
		m_Active[index] = true;
		return true;
	}
	else if (result != SCE_OK) {
		return false;
	}

	if (m_Active[index] && isMaDeviceWorking(&m_Controller[index])) {
		return false;
	}
	else if (m_Active[index] && !isMaDeviceWorking(&m_Controller[index])) {
		ma_device_uninit(&m_Controller[index]);
	}

	{
		std::lock_guard<std::mutex> lock(m_BufferMutex);
		for (uint32_t i = 0; i < 4; i++)
		{
			m_AudioBuffer[i].clear();
		}
	}

	s_ScePadContainerIdInfo info = {};
	result = scePadGetContainerIdInformation(handle, &info);

	if (result != SCE_OK)
	{
		return false;
	}

#ifdef WINDOWS
	GUID targetContainerId = StringToGuid(info.id);

	if (strlen(info.id) == 0)
		return false;

	{
		std::lock_guard<std::mutex> guard(g_contextLock);
		result = FindDeviceByContainerIdWindows(&g_context, targetContainerId, &m_ControllerId[index]);
		if (result != MA_SUCCESS)
			return false;
	}
#else
	if (strlen(info.id) == 0)
		return false;

	{
		std::lock_guard<std::mutex> guard(g_contextLock);
		std::string id = std::string(info.id, strlen(info.id));
		result = FindDeviceByContainerIdLinux(&g_context, id, &m_ControllerId[index]);
		if (result != MA_SUCCESS)
			return false;
	}
#endif

	s_SceControllerType controllerType = {};
	scePadGetControllerType(handle, &controllerType);

	ma_device_config playbackConfig = ma_device_config_init(ma_device_type_playback);
	playbackConfig.playback.format = ma_format_f32;
	playbackConfig.playback.channels = 0;
	playbackConfig.sampleRate = 48000;
	playbackConfig.dataCallback = controllerType == s_SceControllerType::DUALSENSE ? PlaybackDualsenseDataCallback : PlaybackDualshock4DataCallback;
	playbackConfig.playback.pDeviceID = &m_ControllerId[index];
	playbackConfig.periodSizeInFrames = 128;
	playbackConfig.periods = 2;
	playbackConfig.pUserData = this;

	result = ma_device_init(NULL, &playbackConfig, &m_Controller[index]);
	if (result != MA_SUCCESS)
	{
		LOGE("Failed to open playback device.");
		ma_device_uninit(&m_Controller[index]);
		return false;
	}

	if (ma_device_start(&m_Controller[index]) != MA_SUCCESS)
	{
		LOGE("Failed to start controller audio device.");
	}

	m_Active[index] = true;
	return true;
}

bool AudioPassthrough::StopByUserId(uint32_t userId)
{
	assert(userId >= 1 && userId <= 4);

	uint32_t index = userId - 1;

	uint32_t handle = scePadGetHandle(userId, 0, 0);
	int busType = -1;
	uint32_t result = scePadGetControllerBusType(handle, &busType);
	if (result == SCE_OK && busType == SCE_PAD_BUSTYPE_BT) {
		m_Active[index] = false;
		return true;
	}
	else if (result != SCE_OK) {
		return false;
	}

	if (m_Active[index])
	{
		if (isMaDeviceWorking(&m_Controller[index]))
		{
			ma_device_stop(&m_Controller[index]);
		}
		ma_device_uninit(&m_Controller[index]);

		{
			std::lock_guard<std::mutex> lock(m_BufferMutex);
			m_AudioBuffer[index].clear();
			m_AudioBuffer[index].shrink_to_fit();
		}
	}

	m_Active[index] = false;

	return true;
}

void AudioPassthrough::SetHapticIntensityByUserId(uint32_t userId, float intensity)
{
	assert(userId >= 1 && userId <= 4);

	m_HapticIntensity[userId - 1] = intensity;
}

float AudioPassthrough::GetCurrentCapturePeak()
{
	return m_CurrentCapturePeak;
}

void AudioPassthrough::SetCaptureDevice(uint32_t Device)
{
	m_CurrentCaptureDevice = Device;
}

std::vector<std::string> AudioPassthrough::GetCaptureDeviceList()
{
	static std::vector<std::string> list;
	ma_device_info *pCapturesInfos;
	ma_uint32 captureCount;
	if (ma_context_get_devices(&g_context, nullptr, nullptr, &pCapturesInfos, &captureCount) != MA_SUCCESS)
	{
		return std::vector<std::string>();
	}

	list.clear();

	for (ma_uint32 iDevice = 0; iDevice < captureCount; iDevice += 1)
	{
		list.push_back(pCapturesInfos[iDevice].name);
	}

	return list;
}
