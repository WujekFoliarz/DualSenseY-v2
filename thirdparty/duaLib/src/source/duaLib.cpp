#include <iostream>
#include <hidapi.h>
#include <stdio.h>
#include <wchar.h>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <atomic>      
#include <thread>       
#include <chrono>      
#include <cstring>    
#include <cmath>
#include <shared_mutex>
#include <fstream>
#include <iomanip> 

#define M_PI 3.14159265358979323846  

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#include <setupapi.h>
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")
#pragma comment(lib, "winmm.lib")
#include <initguid.h>
#include <devpkey.h>
#include <hidsdi.h>
#include <codecvt>
#include <locale>
#else
#include <clocale>
#include <cstdlib>
#endif

#include "duaLib.h"
#include "dataStructures.h"
#include "crc.h"
#include "triggerFactory.h"

#define DEVICE_COUNT 4
#define MAX_CONTROLLER_COUNT 4
#define VENDOR_ID 0x54c
#define DUALSENSE_DEVICE_ID 0x0ce6
#define DUALSENSE_EDGE_DEVICE_ID 0x0df2
#define DUALSHOCK4_DEVICE_ID 0x05c4
#define DUALSHOCK4V2_DEVICE_ID 0x09cc
#define DUALSHOCK4_WIRELESS_ADAPTOR_ID 0xba0
#define UNKNOWN 0
#define DUALSHOCK4 1
#define DUALSENSE 2
#define ANGULAR_VELOCITY_DEADBAND_MIN 0.017453292

namespace duaLibUtils {
	struct trigger {
		uint8_t force[11] = {};
	};

	struct controller {
		std::shared_mutex lock{};
		hid_device* handle = 0;
		uint32_t sceHandle = 0;
		uint8_t playerIndex = 0;
		uint8_t deviceType = UNKNOWN;
		uint16_t productID = 0;
		uint8_t seqNo = 0;
		uint8_t connectionType = 0;
		bool opened = false;
		bool isMicMuted = false;
		bool wasDisconnected = false;
		bool valid = false;
		uint32_t failedReadCount = 0;
		dualsenseData::USBGetStateData dualsenseCurInputState = {};
		dualsenseData::SetStateData dualsenseLastOutputState = {};
		dualsenseData::SetStateData dualsenseCurOutputState = {};
		dualsenseData::ReportFeatureInVersion versionReport = {};
		dualshock4Data::USBGetStateData dualshock4CurInputState = {};
		dualshock4Data::BTSetStateData dualshock4LastOutputState = {};
		dualshock4Data::BTSetStateData dualshock4CurOutputState = {};
		dualshock4Data::ReportFeatureInDongleSetAudio dualshock4CurAudio = { 0xE0, 0, dualshock4Data::AudioOutput::Disabled };
		dualshock4Data::ReportFeatureInDongleSetAudio dualshock4LastAudio = { 0xE0, 0, dualshock4Data::AudioOutput::Speaker };
		std::string macAddress = "";
		std::string systemIdentifier = "";
		std::string lastPath = "";
		std::string id = "";
		uint32_t idSize = 0;
		trigger L2 = {};
		trigger R2 = {};
		uint8_t triggerMask = 0;
		uint32_t lastSensorTimestamp = 0;
		bool velocityDeadband = false;
		bool motionSensorState = true;
		bool tiltCorrection = false;
		s_SceFQuaternion orientation = { 0.0f,0.0f,0.0f,1.0f };
		s_SceFVector3 lastAcceleration = { 0.0f,0.0f,0.0f };
		float eInt[3] = { 0.0f, 0.0f, 0.0f };
		std::chrono::steady_clock::time_point lastUpdate = {};
		float deltaTime = 0.0f;
		uint8_t touch1Count = 0;
		uint8_t touch2Count = 0;
		uint8_t touch1LastCount = 0;
		uint8_t touch2LastCount = 0;
		uint8_t touch1LastIndex = 0;
		uint8_t touch2LastIndex = 0;
		bool started = false;
		bool playerLed = true;
	};

	void setPlayerLights(duaLibUtils::controller& controller, bool oldStyle) {
		if (!controller.playerLed) {
			controller.dualsenseCurOutputState.PlayerLight1 = false;
			controller.dualsenseCurOutputState.PlayerLight2 = false;
			controller.dualsenseCurOutputState.PlayerLight3 = false;
			controller.dualsenseCurOutputState.PlayerLight4 = false;
			controller.dualsenseCurOutputState.PlayerLight5 = false;
			return;
		}

		switch (controller.playerIndex) {
			case 1:
				controller.dualsenseCurOutputState.PlayerLight1 = false;
				controller.dualsenseCurOutputState.PlayerLight2 = false;
				controller.dualsenseCurOutputState.PlayerLight3 = true;
				controller.dualsenseCurOutputState.PlayerLight4 = false;
				controller.dualsenseCurOutputState.PlayerLight5 = false;
				break;

			case 2:
				controller.dualsenseCurOutputState.PlayerLight1 = false;
				controller.dualsenseCurOutputState.PlayerLight2 = true; // Simplified, as oldStyle ? true : true is always true
				controller.dualsenseCurOutputState.PlayerLight3 = false;
				controller.dualsenseCurOutputState.PlayerLight4 = oldStyle ? true : false;
				controller.dualsenseCurOutputState.PlayerLight5 = false;
				break;

			case 3:
				controller.dualsenseCurOutputState.PlayerLight1 = true;
				controller.dualsenseCurOutputState.PlayerLight2 = false;
				controller.dualsenseCurOutputState.PlayerLight3 = true;
				controller.dualsenseCurOutputState.PlayerLight4 = false;
				controller.dualsenseCurOutputState.PlayerLight5 = oldStyle ? true : false;
				break;

			case 4:
				controller.dualsenseCurOutputState.PlayerLight1 = true;
				controller.dualsenseCurOutputState.PlayerLight2 = true;
				controller.dualsenseCurOutputState.PlayerLight3 = false;
				controller.dualsenseCurOutputState.PlayerLight4 = oldStyle ? true : false;
				controller.dualsenseCurOutputState.PlayerLight5 = oldStyle ? true : false;
				break;

			default:
				controller.dualsenseCurOutputState.PlayerLight1 = false;
				controller.dualsenseCurOutputState.PlayerLight2 = false;
				controller.dualsenseCurOutputState.PlayerLight3 = false;
				controller.dualsenseCurOutputState.PlayerLight4 = false;
				controller.dualsenseCurOutputState.PlayerLight5 = false;
				break;
		}
	}

	bool letGo(hid_device* handle, uint8_t deviceType, uint8_t connectionType) {
		if (handle && deviceType == DUALSENSE && (connectionType == HID_API_BUS_USB || connectionType == HID_API_BUS_UNKNOWN)) {
			dualsenseData::ReportOut02 data = {};
			data.ReportID = 0x02;

			data.State.ResetLights = true;
			data.State.AllowLedColor = false;
			data.State.AllowAudioControl = false;
			data.State.AllowAudioControl2 = false;
			data.State.AllowAudioMute = true;
			data.State.MicMute = false;
			data.State.AllowColorLightFadeAnimation = false;
			data.State.AllowHapticLowPassFilter = false;
			data.State.AllowHeadphoneVolume = false;
			data.State.AllowLightBrightnessChange = false;
			data.State.AllowMicVolume = false;
			data.State.AllowMotorPowerLevel = false;
			data.State.AllowMuteLight = false;
			data.State.AllowPlayerIndicators = false;
			data.State.AllowSpeakerVolume = false;
			data.State.UseRumbleNotHaptics = false;
			data.State.RumbleEmulationLeft = 0;
			data.State.RumbleEmulationRight = 0;

			data.State.AllowLeftTriggerFFB = true;
			data.State.AllowRightTriggerFFB = true;
			TriggerEffectGenerator::Off(data.State.LeftTriggerFFB, 0);
			TriggerEffectGenerator::Off(data.State.RightTriggerFFB, 0);

			uint8_t res = hid_write(handle, reinterpret_cast<unsigned char*>(&data), sizeof(data));
			return true;
		}
		else if (handle && deviceType == DUALSENSE && connectionType == HID_API_BUS_BLUETOOTH) {
			dualsenseData::ReportOut31 data = {};

			data.Data.ReportID = 0x31;
			data.Data.flag = 2;
			data.Data.State.ResetLights = true;
			uint32_t crc = compute(data.CRC.Buff, sizeof(data) - 4);
			data.CRC.CRC = crc;
			uint8_t res = hid_write(handle, reinterpret_cast<unsigned char*>(&data), sizeof(data));

			data.Data.State.ResetLights = false;
			data.Data.State.AllowLedColor = true;
			data.Data.State.AllowLeftTriggerFFB = true;
			data.Data.State.AllowRightTriggerFFB = true;
			data.Data.State.LedBlue = 255;
			TriggerEffectGenerator::Off(data.Data.State.LeftTriggerFFB, 0);
			TriggerEffectGenerator::Off(data.Data.State.RightTriggerFFB, 0);
			crc = compute(data.CRC.Buff, sizeof(data) - 4);
			data.CRC.CRC = crc;
			res = hid_write(handle, reinterpret_cast<unsigned char*>(&data), sizeof(data));

			return true;
		}
		if (handle && deviceType == DUALSHOCK4 && (connectionType == HID_API_BUS_USB || connectionType == HID_API_BUS_UNKNOWN)) {
			dualshock4Data::ReportIn05 report = {};
			report.ReportID = 0x05;
			report.State.UNK_RESET1 = true;
			report.State.UNK_RESET2 = true;
			report.State.EnableRumbleUpdate = true;
			report.State.RumbleLeft = 0;
			report.State.RumbleRight = 0;
			hid_write(handle, reinterpret_cast<unsigned char*>(&report), sizeof(report));

			dualshock4Data::ReportFeatureInDongleSetAudio audioSetting = {};
			audioSetting.ReportID = 0xE0;
			audioSetting.Output = dualshock4Data::AudioOutput::Disabled;
			hid_send_feature_report(handle, reinterpret_cast<unsigned char*>(&audioSetting), sizeof(audioSetting));

			return true;
		}
		else if (handle && deviceType == DUALSHOCK4 && connectionType == HID_API_BUS_BLUETOOTH) {
			dualshock4Data::ReportOut11 report = {};
			report.Data.ReportID = 0x11;
			report.Data.EnableHID = 1;
			report.Data.AllowRed = 1;
			report.Data.AllowGreen = 1;
			report.Data.AllowBlue = 1;
			report.Data.EnableAudio = 0;
			report.Data.State.LedRed = 50;
			report.Data.State.LedGreen = 50;
			report.Data.State.LedBlue = 50;
			report.Data.State.EnableLedUpdate = true;
			report.Data.State.EnableRumbleUpdate = true;
			report.Data.State.RumbleLeft = 0;
			report.Data.State.RumbleRight = 0;

			uint32_t crc = compute(report.CRC.Buff, sizeof(report) - 4);
			report.CRC.CRC = crc;

			int res = hid_write(handle, reinterpret_cast<unsigned char*>(&report), sizeof(report));
			return true;
		}

		return false;
	}

	bool getHardwareVersion(hid_device* handle, dualsenseData::ReportFeatureInVersion& report) {
		if (!handle) return false;

		unsigned char buffer[64] = { };
		buffer[0] = 0x20;
		int res = hid_get_feature_report(handle, buffer, sizeof(buffer));

		if (res > 0) {
			const auto versionReport = *reinterpret_cast<dualsenseData::ReportFeatureInVersion*>(buffer);
			report = versionReport;
			return true;
		}

		return false;
	}

	bool getMacAddress(hid_device* handle, std::string& outMac, uint32_t deviceId, uint8_t connectionType) {
		if (!handle) return false;

		if (deviceId == DUALSENSE_DEVICE_ID || deviceId == DUALSENSE_EDGE_DEVICE_ID) {
			unsigned char buffer[20] = {};
			buffer[0] = 0x09; // Report ID
			int res = hid_get_feature_report(handle, buffer, sizeof(buffer));

			if (res > 0) {
				const auto macReport = *reinterpret_cast<dualsenseData::ReportFeatureInMacAll*>(buffer);
				char tmp[18];
				snprintf(tmp, sizeof(tmp), "%02X:%02X:%02X:%02X:%02X:%02X",
						 macReport.ClientMac[5], macReport.ClientMac[4], macReport.ClientMac[3],
						 macReport.ClientMac[2], macReport.ClientMac[1], macReport.ClientMac[0]);
				outMac = tmp;				
				return true;
			}
		}
		else if ((deviceId == DUALSHOCK4_DEVICE_ID || deviceId == DUALSHOCK4V2_DEVICE_ID || deviceId == DUALSHOCK4_WIRELESS_ADAPTOR_ID) && (connectionType == HID_API_BUS_USB || connectionType == HID_API_BUS_UNKNOWN)) {
			dualshock4Data::ReportFeatureInMacAll macReport = {};
			macReport.ReportID = 0x12;
			int res = hid_get_feature_report(handle, reinterpret_cast<unsigned char*>(&macReport), sizeof(macReport));

			if (res > 0) {
				char tmp[18];
				snprintf(tmp, sizeof(tmp), "%02X:%02X:%02X:%02X:%02X:%02X",
						 macReport.ClientMac[5], macReport.ClientMac[4], macReport.ClientMac[3],
						 macReport.ClientMac[2], macReport.ClientMac[1], macReport.ClientMac[0]);
				outMac = tmp;
				return true;
			}
		}
		else if ((deviceId == DUALSHOCK4_DEVICE_ID || deviceId == DUALSHOCK4V2_DEVICE_ID || deviceId == DUALSHOCK4_WIRELESS_ADAPTOR_ID) && connectionType == HID_API_BUS_BLUETOOTH) {
			dualshock4Data::ReportFeatureInMacAllBT macReport = {};
			macReport.Data.ReportID = 0x09;
			int res = hid_get_feature_report(handle, reinterpret_cast<unsigned char*>(&macReport), sizeof(macReport));

			if (res > 0) {
				char tmp[18];
				snprintf(tmp, sizeof(tmp), "%02X:%02X:%02X:%02X:%02X:%02X",
						 macReport.Data.ClientMac[5], macReport.Data.ClientMac[4], macReport.Data.ClientMac[3],
						 macReport.Data.ClientMac[2], macReport.Data.ClientMac[1], macReport.Data.ClientMac[0]);
				outMac = tmp;

				return true;
			}
		}

		return false;
	}

	bool isValid(hid_device* handle) {
		if (!handle) return false;
		unsigned char buf[1] = {};
		return hid_read(handle, buf, 1) != -1;
	}

#if defined(_WIN32) || defined(_WIN64)
	static std::wstring Utf8ToWide(const char* utf8) {
		int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
		if (wlen <= 0) return L"";
		std::vector<wchar_t> buf(wlen);
		MultiByteToWideChar(CP_UTF8, 0, utf8, -1, buf.data(), wlen);
		return std::wstring(buf.data());
	}
#endif

	bool GetID(const char* narrowPath, const char** ID, uint32_t* size) {
	#if defined(_WIN32) || defined(_WIN64)
		GUID hidGuid;
		GUID outContainerId;
		HidD_GetHidGuid(&hidGuid);

		HDEVINFO devs = SetupDiGetClassDevs(
			&hidGuid,
			nullptr,
			nullptr,
			DIGCF_DEVICEINTERFACE | DIGCF_PRESENT
		);
		if (devs == INVALID_HANDLE_VALUE) {
			return false;
		}

		SP_DEVICE_INTERFACE_DATA ifData = { sizeof(ifData) };
		DWORD index = 0;
		std::wstring targetPath = Utf8ToWide(narrowPath);
		std::transform(targetPath.begin(), targetPath.end(), targetPath.begin(), ::tolower);

		while (SetupDiEnumDeviceInterfaces(devs, nullptr, &hidGuid, index++, &ifData)) {
			DWORD needed = 0;
			SetupDiGetDeviceInterfaceDetailW(devs, &ifData, nullptr, 0, &needed, nullptr);
			auto detailBuf = (SP_DEVICE_INTERFACE_DETAIL_DATA_W*)malloc(needed);
			detailBuf->cbSize = sizeof(*detailBuf);
			SP_DEVINFO_DATA devInfo = { sizeof(devInfo) };

			if (SetupDiGetDeviceInterfaceDetailW(
				devs, &ifData,
				detailBuf, needed,
				nullptr,
				&devInfo
				)) {
				if (targetPath == detailBuf->DevicePath) {
					DEVPROPTYPE propType = 0;
					DWORD cb = sizeof(GUID);
					if (SetupDiGetDevicePropertyW(
						devs,
						&devInfo,
						&DEVPKEY_Device_ContainerId,
						&propType,
						reinterpret_cast<PBYTE>(&outContainerId),
						cb,
						&cb,
						0
						)) {
						free(detailBuf);
						SetupDiDestroyDeviceInfoList(devs);

						wchar_t guidStr[39] = {};
						StringFromGUID2(outContainerId, guidStr, _countof(guidStr));

						*size = sizeof(guidStr);
						static char buffer[39] = {};
						std::wcstombs(buffer, guidStr, sizeof(buffer));
						std::transform(buffer, buffer + std::strlen(buffer), buffer, [](unsigned char c) {return std::tolower(c); });
						*ID = buffer;

						return true;
					}
				}
			}
			free(detailBuf);
		}

		SetupDiDestroyDeviceInfoList(devs);
	#endif
		return false;
	}
}

struct device {
	uint16_t Vendor = 0;
	uint16_t Device = 0;
};

struct deviceList {
	device devices[DEVICE_COUNT];

	deviceList() {
		devices[0] = { VENDOR_ID, DUALSENSE_DEVICE_ID };
		devices[1] = { VENDOR_ID, DUALSENSE_EDGE_DEVICE_ID };
		devices[2] = { VENDOR_ID, DUALSHOCK4_DEVICE_ID };
		devices[3] = { VENDOR_ID, DUALSHOCK4V2_DEVICE_ID };
	}
};

deviceList g_deviceList = {};
duaLibUtils::controller g_controllers[MAX_CONTROLLER_COUNT] = {};
static std::atomic<bool> g_threadRunning = false;
static std::atomic<bool> g_initialized = false;
static std::atomic<bool> g_particularMode = false;
static std::atomic<bool> g_allowBluetooth = false;
static std::thread g_readThread;
static std::thread g_watchThread;
constexpr std::array<s_SceLightBar, 4> g_playerColors = { {
	{  0, 0, 255 }, // Player 1 - Blue
	{255,  0,   0 }, // Player 2 - Red
	{  0, 255, 0 }, // Player 3 - Green
	{255, 0, 255 }  // Player 4 - Pink
} };

int readFunc() {
#if defined(_WIN32) || defined(_WIN64)
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	timeBeginPeriod(1);

	EXECUTION_STATE prevState = SetThreadExecutionState(
		ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED
	);

	HANDLE hTimer = CreateWaitableTimerEx(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
	LARGE_INTEGER liDueTime;
	liDueTime.QuadPart = -5000LL;
#endif

	while (g_threadRunning) {
		bool allInvalid = true;		
		
		for (auto& controller : g_controllers) {
			if (controller.valid && controller.opened && controller.deviceType == DUALSENSE) {
				allInvalid = false;
				bool isBt = controller.connectionType == HID_API_BUS_BLUETOOTH ? true : false;

				dualsenseData::ReportIn01USB  inputUsb = {};
				dualsenseData::ReportIn31  inputBt = {};

				int32_t res = -1;

				if (isBt) 
					res = hid_read_timeout(controller.handle, reinterpret_cast<unsigned char*>(&inputBt), sizeof(inputBt), 0);			
				else 
					res = hid_read_timeout(controller.handle, reinterpret_cast<unsigned char*>(&inputUsb), sizeof(inputUsb), 0);	

				dualsenseData::USBGetStateData inputData = isBt ? inputBt.Data.State.StateData : inputUsb.State;

				if (controller.failedReadCount >= 15) {
					controller.valid = false;
				}

				if (res == -1) {
					controller.failedReadCount++;
					continue;
				}
				else if (res > 0) {
					controller.failedReadCount = 0;

					if (!inputData.ButtonMute && controller.dualsenseCurInputState.ButtonMute) {
						controller.dualsenseCurOutputState.AllowAudioMute = true;
						controller.isMicMuted = !controller.isMicMuted;
						controller.dualsenseCurOutputState.MuteLightMode = controller.isMicMuted ? dualsenseData::MuteLight::On : dualsenseData::MuteLight::Off;
						controller.dualsenseCurOutputState.MicMute = controller.isMicMuted;
						controller.dualsenseCurOutputState.AllowMuteLight = true;
					}
					else {
						controller.dualsenseCurOutputState.AllowAudioMute = false;
						controller.dualsenseCurOutputState.AllowMuteLight = false;
					}

					if (controller.dualsenseCurOutputState.LedRed != controller.dualsenseLastOutputState.LedRed ||
						controller.dualsenseCurOutputState.LedGreen != controller.dualsenseLastOutputState.LedGreen ||
						controller.dualsenseCurOutputState.LedBlue != controller.dualsenseLastOutputState.LedBlue ||
						controller.wasDisconnected) {
						controller.dualsenseCurOutputState.AllowLedColor = true;
					}
					else {
						controller.dualsenseCurOutputState.AllowLedColor = false;
					}

					bool oldStyle = ((controller.versionReport.HardwareInfo & 0x00FFFF00) < 0x00000400);
					duaLibUtils::setPlayerLights(controller, oldStyle);

					if (controller.dualsenseCurOutputState.lightBrightness != controller.dualsenseLastOutputState.lightBrightness || controller.wasDisconnected) {
						controller.dualsenseCurOutputState.AllowLightBrightnessChange = true;
					}
					else {
						controller.dualsenseCurOutputState.AllowLightBrightnessChange = false;
					}

					if ((controller.dualsenseCurOutputState.PlayerLight1 != controller.dualsenseLastOutputState.PlayerLight1 ||
						controller.dualsenseCurOutputState.PlayerLight2 != controller.dualsenseLastOutputState.PlayerLight2 ||
						controller.dualsenseCurOutputState.PlayerLight3 != controller.dualsenseLastOutputState.PlayerLight3 ||
						controller.dualsenseCurOutputState.PlayerLight4 != controller.dualsenseLastOutputState.PlayerLight4) || controller.wasDisconnected) {
						controller.dualsenseCurOutputState.AllowPlayerIndicators = true;
					}
					else {
						controller.dualsenseCurOutputState.AllowPlayerIndicators = true; // keep on true because it doesn't always light up
					}

					if (controller.wasDisconnected) {
						controller.dualsenseCurOutputState.MicMute = controller.isMicMuted;
						controller.dualsenseCurOutputState.AllowMuteLight = true;
					}

					if (controller.dualsenseCurOutputState.OutputPathSelect != controller.dualsenseLastOutputState.OutputPathSelect ||
						controller.wasDisconnected) {
						controller.dualsenseCurOutputState.AllowAudioControl = true;
					}
					else {
						controller.dualsenseCurOutputState.AllowAudioControl = false;
					}

					if (controller.dualsenseCurOutputState.VolumeSpeaker != controller.dualsenseLastOutputState.VolumeSpeaker ||
						controller.wasDisconnected) {
						controller.dualsenseCurOutputState.AllowSpeakerVolume = true;
					}
					else {
						controller.dualsenseCurOutputState.AllowSpeakerVolume = false;
					}

					if (controller.dualsenseCurOutputState.VolumeMic != controller.dualsenseLastOutputState.VolumeMic ||
						controller.wasDisconnected) {
						controller.dualsenseCurOutputState.AllowMicVolume = true;
					}
					else {
						controller.dualsenseCurOutputState.AllowMicVolume = false;
					}

					if (controller.dualsenseCurOutputState.VolumeHeadphones != controller.dualsenseLastOutputState.VolumeHeadphones ||
						controller.wasDisconnected) {
						controller.dualsenseCurOutputState.AllowHeadphoneVolume = true;
					}
					else {
						controller.dualsenseCurOutputState.AllowHeadphoneVolume = false;
					}

					if (controller.triggerMask & SCE_PAD_TRIGGER_EFFECT_TRIGGER_MASK_L2 || controller.wasDisconnected) {
						controller.dualsenseCurOutputState.AllowLeftTriggerFFB = true;
						for (int i = 0; i < 11; i++) {
							controller.dualsenseCurOutputState.LeftTriggerFFB[i] = controller.L2.force[i];
						}
					}
					else {
						controller.dualsenseCurOutputState.AllowLeftTriggerFFB = false;
					}

					if (controller.triggerMask & SCE_PAD_TRIGGER_EFFECT_TRIGGER_MASK_R2 || controller.wasDisconnected) {
						controller.dualsenseCurOutputState.AllowRightTriggerFFB = true;
						for (int i = 0; i < 11; i++) {
							controller.dualsenseCurOutputState.RightTriggerFFB[i] = controller.R2.force[i];
						}
					}
					else {
						controller.dualsenseCurOutputState.AllowRightTriggerFFB = false;
					}

					controller.dualsenseCurOutputState.HostTimestamp = controller.dualsenseCurInputState.SensorTimestamp;
					res = -1;

					if (controller.connectionType == HID_API_BUS_USB || controller.connectionType == HID_API_BUS_UNKNOWN) {
						dualsenseData::ReportOut02 usbOutput = {};

						usbOutput.ReportID = 0x02;
						usbOutput.State = controller.dualsenseCurOutputState;

						if ((controller.dualsenseCurOutputState != controller.dualsenseLastOutputState) || controller.wasDisconnected) {
							res = hid_write(controller.handle, reinterpret_cast<unsigned char*>(&usbOutput), sizeof(usbOutput));
						}
					}
					else if (controller.connectionType == HID_API_BUS_BLUETOOTH) {
						dualsenseData::ReportOut31 btOutput = {};

						btOutput.Data.ReportID = 0x31;
						btOutput.Data.flag = 2;
						btOutput.Data.State = controller.dualsenseCurOutputState;

						uint32_t crc = compute(btOutput.CRC.Buff, sizeof(btOutput) - 4);
						btOutput.CRC.CRC = crc;
						if ((controller.dualsenseCurOutputState != controller.dualsenseLastOutputState) || controller.wasDisconnected) {
							res = hid_write(controller.handle, reinterpret_cast<unsigned char*>(&btOutput), sizeof(btOutput));
						}
					}

					if (res > 0) {
						std::shared_lock guard(controller.lock);
						controller.wasDisconnected = false;
						//std::cout << "Controller idx " << controller.sceHandle << " path=" << controller.macAddress << " connType=" << (int)controller.connectionType << std::endl;
					}

					{
						std::shared_lock guard(controller.lock);
						controller.dualsenseLastOutputState = controller.dualsenseCurOutputState;
						controller.dualsenseCurInputState = inputData;
					}
				}
			}
			else if (controller.valid && controller.opened && controller.deviceType == DUALSHOCK4) {
				allInvalid = false;
				bool isBt = controller.connectionType == HID_API_BUS_BLUETOOTH ? true : false;

				dualshock4Data::ReportIn01USB inputUsb = {};
				dualshock4Data::ReportIn01BT inputBt = {};

				int32_t res = -1;

				if (isBt)
					res = hid_read_timeout(controller.handle, reinterpret_cast<unsigned char*>(&inputBt), sizeof(inputBt), 0);
				else
					res = hid_read_timeout(controller.handle, reinterpret_cast<unsigned char*>(&inputUsb), sizeof(inputUsb), 0);

				if (controller.failedReadCount >= 15) {
					controller.valid = false;
				}

				if (res == -1) {
					controller.failedReadCount++;
					continue;
				}
				else if (res > 0) {
					controller.failedReadCount = 0;

					if (controller.dualshock4CurOutputState.LedRed != controller.dualshock4LastOutputState.LedRed ||
					controller.dualshock4CurOutputState.LedGreen != controller.dualshock4LastOutputState.LedGreen ||
					controller.dualshock4CurOutputState.LedBlue != controller.dualshock4LastOutputState.LedBlue ||
					controller.wasDisconnected) {
						controller.dualshock4CurOutputState.EnableLedUpdate = true;
					}
					else {
						controller.dualshock4CurOutputState.EnableLedUpdate = true;
					}

					if (controller.dualshock4CurAudio.Output != controller.dualshock4LastAudio.Output || controller.wasDisconnected) {
						controller.dualshock4CurAudio.ReportID = 0xE0;
						hid_send_feature_report(controller.handle, reinterpret_cast<unsigned char*>(&controller.dualshock4CurAudio), sizeof(controller.dualshock4CurAudio));
						controller.dualshock4LastAudio.Output = controller.dualshock4CurAudio.Output;
					}

					if (controller.dualshock4CurOutputState.VolumeSpeaker != controller.dualshock4LastOutputState.VolumeSpeaker || controller.wasDisconnected)
						controller.dualshock4CurOutputState.EnableVolumeSpeakerUpdate = true;
					else
						controller.dualshock4CurOutputState.EnableVolumeSpeakerUpdate = false;

					if (controller.dualshock4CurOutputState.VolumeMic != controller.dualshock4LastOutputState.VolumeMic || controller.wasDisconnected)
						controller.dualshock4CurOutputState.EnableVolumeMicUpdate = true;
					else
						controller.dualshock4CurOutputState.EnableVolumeMicUpdate = false;

					if (controller.dualshock4CurOutputState.VolumeLeft != controller.dualshock4LastOutputState.VolumeLeft || controller.wasDisconnected)
						controller.dualshock4CurOutputState.EnableVolumeLeftUpdate = true;
					else
						controller.dualshock4CurOutputState.EnableVolumeLeftUpdate = false;

					if (controller.dualshock4CurOutputState.VolumeRight != controller.dualshock4LastOutputState.VolumeRight || controller.wasDisconnected)
						controller.dualshock4CurOutputState.EnableVolumeRightUpdate = true;
					else
						controller.dualshock4CurOutputState.EnableVolumeRightUpdate = false;

					if ((controller.dualshock4CurOutputState.RumbleLeft != controller.dualshock4LastOutputState.RumbleLeft) || (controller.dualshock4CurOutputState.RumbleRight != controller.dualshock4LastOutputState.RumbleRight) || controller.wasDisconnected)
						controller.dualshock4CurOutputState.EnableRumbleUpdate = true;
					else
						controller.dualshock4CurOutputState.EnableRumbleUpdate = false;

					res = -1;

					if (controller.connectionType == HID_API_BUS_USB || controller.connectionType == HID_API_BUS_UNKNOWN) {
						dualshock4Data::ReportIn05 usbOutput = {};

						usbOutput.ReportID = 0x05;
						usbOutput.State = controller.dualshock4CurOutputState;

						if ((controller.dualshock4CurOutputState != controller.dualshock4LastOutputState) || controller.wasDisconnected) {
							res = hid_write(controller.handle, reinterpret_cast<unsigned char*>(&usbOutput), sizeof(usbOutput));
						}
					}
					else if (controller.connectionType == HID_API_BUS_BLUETOOTH) {
						dualshock4Data::ReportOut11 report = {};
						report.Data.ReportID = 0x11;
						report.Data.EnableHID = 1;
						report.Data.AllowRed = controller.dualshock4CurOutputState.LedRed > 0 ? 1 : 0;
						report.Data.AllowGreen = controller.dualshock4CurOutputState.LedGreen > 0 ? 1 : 0;
						report.Data.AllowBlue = controller.dualshock4CurOutputState.LedBlue > 0 ? 1 : 0;
						report.Data.EnableAudio = 0;
						report.Data.State = controller.dualshock4CurOutputState;

						uint32_t crc = compute(report.CRC.Buff, sizeof(report) - 4);
						report.CRC.CRC = crc;

						int res = hid_write(controller.handle, reinterpret_cast<unsigned char*>(&report), sizeof(report));
					}

					if (res > 0) {
						controller.wasDisconnected = false;
						//std::cout << "Controller idx " << controller.sceHandle << " path=" << controller.macAddress << " connType=" << (int)controller.connectionType << std::endl;
					}

					{
						std::shared_lock guard(controller.lock);
						controller.dualshock4LastOutputState = controller.dualshock4CurOutputState;
						controller.dualshock4CurInputState = isBt ? inputBt.State : inputUsb.State;
					}
				}
			}
			else if (!controller.valid && controller.opened) {
				std::shared_lock guard(controller.lock);
				controller.wasDisconnected = true;
			}
		}

		if (allInvalid) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

	#if defined(_WIN32) || defined(_WIN64)
		SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0);
		WaitForSingleObject(hTimer, INFINITE);
	#else
		std::this_thread::sleep_for(std::chrono::microseconds(100));
	#endif
	}
	return 0;
}

int watchFunc() {
#if defined(_WIN32) || defined(_WIN64)
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
#endif

	while (g_threadRunning) {
		std::this_thread::sleep_for(std::chrono::seconds(1));

		for (int j = 0; j < DEVICE_COUNT; ++j) {
			hid_device_info* head = hid_enumerate(
				g_deviceList.devices[j].Vendor,
				g_deviceList.devices[j].Device
			);

			for (hid_device_info* info = head; info; info = info->next) {
				std::string newMac;
				bool already = false;
				bool invalid = false;
				bool started = false;

				hid_device* handle = hid_open_path(info->path);

				if (info->bus_type == HID_API_BUS_BLUETOOTH && !g_allowBluetooth) {
					hid_close(handle);
					goto skipController;
				}

				if (!handle) continue;

				if (duaLibUtils::getMacAddress(handle, newMac, g_deviceList.devices[j].Device, info->bus_type)) {
					
					// Ignore ViGEm controllers
					if (newMac.rfind("C0:13:37") != std::string::npos) {
						hid_close(handle);
						continue;
					}

					for (int k = 0; k < MAX_CONTROLLER_COUNT; ++k) {
						std::shared_lock guard(g_controllers[k].lock);
						if (g_controllers[k].macAddress == newMac && duaLibUtils::isValid(g_controllers[k].handle)) {
							already = true;
							hid_close(handle);
							break;
						}
					}

					// Restore half valid controllers
					for (auto& controller : g_controllers) {				
						if (duaLibUtils::isValid(controller.handle) && !controller.valid) {
							std::shared_lock guard(controller.lock);
							controller.valid = true;
						}
					}

					if (!already) {
						for (auto& controller : g_controllers) {
							bool valid;
							{
								std::shared_lock guard(controller.lock);
								valid = duaLibUtils::isValid(controller.handle);
							}

							if (!valid) {

								std::shared_lock guard(controller.lock);
								controller.started = true;
								controller.handle = handle;
								controller.macAddress = newMac;
								controller.connectionType = info->bus_type;
								controller.valid = true;
								controller.failedReadCount = 0;
								controller.lastPath = info->path;
								controller.productID = g_deviceList.devices[j].Device;
								hid_set_nonblocking(controller.handle, true);

								const char* id = {};
								uint32_t size = 0;
								duaLibUtils::GetID(info->path, &id, &size);

							#if defined(_WIN32) || defined(_WIN64)
								controller.id = id;
								controller.idSize = size;
							#endif

								uint16_t dev = g_deviceList.devices[j].Device;

								if (dev == DUALSENSE_DEVICE_ID || dev == DUALSENSE_EDGE_DEVICE_ID) { controller.deviceType = DUALSENSE; }
								else if (dev == DUALSHOCK4_DEVICE_ID || dev == DUALSHOCK4V2_DEVICE_ID || dev == DUALSHOCK4_WIRELESS_ADAPTOR_ID) { controller.deviceType = DUALSHOCK4; }

								if (controller.deviceType == DUALSENSE && info->bus_type == HID_API_BUS_BLUETOOTH) {
									duaLibUtils::getHardwareVersion(controller.handle, controller.versionReport);
									dualsenseData::ReportOut31 report = {};

									report.Data.ReportID = 0x31;
									report.Data.flag = 2;
									report.Data.State.EnableRumbleEmulation = true;
									report.Data.State.UseRumbleNotHaptics = true;
									report.Data.State.AllowRightTriggerFFB = true;
									report.Data.State.AllowLeftTriggerFFB = true;
									report.Data.State.AllowLedColor = true;
									report.Data.State.AllowColorLightFadeAnimation = true;
									report.Data.State.lightFadeAnimation = dualsenseData::LightFadeAnimation::FadeOut;
									report.Data.State.ResetLights = true;
									report.Data.State.LeftTriggerFFB[0] = (uint8_t)TriggerEffectType::Off;
									report.Data.State.RightTriggerFFB[0] = (uint8_t)TriggerEffectType::Off;

									uint32_t crc = compute(report.CRC.Buff, sizeof(report) - 4);
									report.CRC.CRC = crc;

									hid_write(controller.handle, reinterpret_cast<unsigned char*>(&report), sizeof(report));
								}
								else if (controller.deviceType == DUALSHOCK4 && info->bus_type == HID_API_BUS_BLUETOOTH) {
									dualshock4Data::ReportOut11 report = {};
									report.Data.ReportID = 0x11;
									report.Data.EnableHID = 1;
									report.Data.AllowRed = 0;
									report.Data.AllowGreen = 0;
									report.Data.AllowBlue = 0;
									report.Data.EnableAudio = 0;
									report.Data.State.LedRed = 0;
									report.Data.State.LedGreen = 0;
									report.Data.State.LedBlue = 0;
									report.Data.State.EnableLedUpdate = true;

									uint32_t crc = compute(report.CRC.Buff, sizeof(report) - 4);
									report.CRC.CRC = crc;

									int res = hid_write(controller.handle, reinterpret_cast<unsigned char*>(&report), sizeof(report));

									unsigned char fullReportFeature[78];
									fullReportFeature[0] = 0x05;
									hid_get_feature_report(controller.handle, fullReportFeature, sizeof(fullReportFeature)); // <-- send this to receive full report
								}

								break;
							}
						}
					}

				skipController:
					{}
				}
			}

			hid_free_enumeration(head);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return 0;
}

int scePadInit() {
	s_ScePadInitParam param = {};
	param.allowBT = false;

	return scePadInit3(&param);
}

int scePadInit3(s_ScePadInitParam* param) {
	if (!param) return SCE_PAD_ERROR_INVALID_ARG;

	if (!g_initialized) {
		int res = hid_init();

		if (res)
			return res;

		for (auto& controller : g_controllers) {
			controller.dualsenseLastOutputState.OutputPathSelect = 10; // Set it to something bigger than 4 so the audio path can reset back to 0 on first write
		}

		g_allowBluetooth = param->allowBT;
		g_threadRunning = true;
		g_readThread = std::thread(readFunc);
		g_watchThread = std::thread(watchFunc);
		g_readThread.detach();
		g_watchThread.detach();
		g_initialized = true;
	}

	return SCE_OK;
}

int scePadTerminate(void) {
	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;
	g_threadRunning = false;
	g_initialized = false;

	for (auto& controller : g_controllers) {
		// release controller here
		controller.valid = false;
		controller.sceHandle = 0;
		controller.lastPath = "";
		controller.productID = 0;
		controller.wasDisconnected = true;
		controller.macAddress = "";

		duaLibUtils::letGo(controller.handle, controller.deviceType, controller.connectionType);
	}
	g_particularMode = false;

	//if (g_readThread.joinable()) {
	//	g_readThread.join();
	//}
	//if (g_watchThread.joinable()) {
	//	g_watchThread.join();
	//}
	return SCE_OK;
}

int scePadOpen(int userID, int unk1, int unk2) {
	volatile int preventOptim1 = unk1;
	volatile int preventOptim2 = unk2;
	(void)preventOptim1;
	(void)preventOptim2;

	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;
	if (userID > MAX_CONTROLLER_COUNT || userID < 1) return SCE_PAD_ERROR_INVALID_ARG;

	int index = userID - 1;
	bool wasAlreadyOpened = false;
	int lastUnused = 0;
	int firstUnused = -1;
	int occupiedCount = 0;
	int count = 0;

	for (auto& controller : g_controllers) {
		std::shared_lock guard(controller.lock);

		if (controller.sceHandle == 0 && controller.playerIndex != userID) {
			if (firstUnused == -1)
				firstUnused = count;

			lastUnused++;
		}
		else if (controller.sceHandle == sizeof(controller) * (occupiedCount + 1) || controller.playerIndex == userID) {
			if (occupiedCount > DEVICE_COUNT) {
				wasAlreadyOpened = true;
				return SCE_PAD_ERROR_ALREADY_OPENED;
			}
		}
		else {
			occupiedCount++;
		}

		count++;
	}

	if (!wasAlreadyOpened) {
		int handle = sizeof(duaLibUtils::controller) * (firstUnused + 1);
		g_controllers[firstUnused].sceHandle = handle;
		g_controllers[firstUnused].opened = true;
		g_controllers[firstUnused].playerIndex = userID;

		g_controllers[firstUnused].dualshock4CurOutputState.LedRed = g_playerColors[userID - 1].r;
		g_controllers[firstUnused].dualshock4CurOutputState.LedGreen = g_playerColors[userID - 1].g;
		g_controllers[firstUnused].dualshock4CurOutputState.LedBlue = g_playerColors[userID - 1].b;

		return handle;
	}

	return SCE_PAD_ERROR_NO_HANDLE;
}

int scePadSetParticularMode(bool mode) {
	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;
	g_particularMode = mode;
	return SCE_OK;
}

int scePadGetParticularMode() {
	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;
	return g_particularMode;
}

static float Vec3Length(const s_SceFVector3& v) {
	return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

static s_SceFVector3 Vec3Normalize(const s_SceFVector3& v) {
	float len = Vec3Length(v);
	if (len > 0.0f) {
		return { v.x / len, v.y / len, v.z / len };
	}
	return v;
}

constexpr float angVelScale = 50.0f / 32767.0f;
constexpr float angVelScaleDS4 = 40.0f / 32767.0f;
// To m/s^2: 0.98 mg/LSB (BMI055 data sheet Chapter 5.2.1)
static double to_mpss(int v) {
	return static_cast<double>(v) / (pow(2, 13) - 1) * 9.80665 * 0.098;
}

// To rad/s: 32767: 2000 deg/s (BMI055 data sheet Chapter 7.2.1)
static double to_radps(int v) {
	return static_cast<double>(v) / (pow(2, 15) - 1) * M_PI / 180.0 * 2000;
}

int scePadReadState(int handle, s_ScePadData* data) {
	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;
	if (!data) return SCE_PAD_ERROR_INVALID_ARG;

	for (auto& controller : g_controllers) {
		std::shared_lock guard(controller.lock);

		if (controller.sceHandle != handle) continue;

		if (!controller.valid) return SCE_PAD_ERROR_DEVICE_NOT_CONNECTED;

		if (controller.deviceType == DUALSENSE) {
		#pragma region buttons
			uint32_t bitmaskButtons = 0;
			if (controller.dualsenseCurInputState.ButtonCross) bitmaskButtons |= SCE_BM_CROSS;
			if (controller.dualsenseCurInputState.ButtonCircle) bitmaskButtons |= SCE_BM_CIRCLE;
			if (controller.dualsenseCurInputState.ButtonTriangle) bitmaskButtons |= SCE_BM_TRIANGLE;
			if (controller.dualsenseCurInputState.ButtonSquare) bitmaskButtons |= SCE_BM_SQUARE;

			if (controller.dualsenseCurInputState.ButtonL1) bitmaskButtons |= SCE_BM_L1;
			if (controller.dualsenseCurInputState.ButtonL2) bitmaskButtons |= SCE_BM_L2;
			if (controller.dualsenseCurInputState.ButtonR1) bitmaskButtons |= SCE_BM_R1;
			if (controller.dualsenseCurInputState.ButtonR2) bitmaskButtons |= SCE_BM_R2;

			if (controller.dualsenseCurInputState.ButtonL3) bitmaskButtons |= SCE_BM_L3;
			if (controller.dualsenseCurInputState.ButtonR3) bitmaskButtons |= SCE_BM_R3;

			if (controller.dualsenseCurInputState.DPad == Direction::NorthEast) bitmaskButtons |= SCE_BM_N_DPAD + SCE_BM_E_DPAD;
			if (controller.dualsenseCurInputState.DPad == Direction::NorthWest) bitmaskButtons |= SCE_BM_N_DPAD + SCE_BM_W_DPAD;
			if (controller.dualsenseCurInputState.DPad == Direction::SouthEast) bitmaskButtons |= SCE_BM_S_DPAD + SCE_BM_E_DPAD;
			if (controller.dualsenseCurInputState.DPad == Direction::SouthWest) bitmaskButtons |= SCE_BM_S_DPAD + SCE_BM_W_DPAD;

			if (controller.dualsenseCurInputState.DPad == Direction::North) bitmaskButtons |= SCE_BM_N_DPAD;
			if (controller.dualsenseCurInputState.DPad == Direction::South) bitmaskButtons |= SCE_BM_S_DPAD;
			if (controller.dualsenseCurInputState.DPad == Direction::East) bitmaskButtons |= SCE_BM_E_DPAD;
			if (controller.dualsenseCurInputState.DPad == Direction::West) bitmaskButtons |= SCE_BM_W_DPAD;

			if (controller.dualsenseCurInputState.ButtonOptions) bitmaskButtons |= SCE_BM_OPTIONS;

			if (controller.dualsenseCurInputState.ButtonPad) bitmaskButtons |= SCE_BM_TOUCH;
			if (controller.dualsenseCurInputState.ButtonMute) bitmaskButtons |= SCE_BM_MICBUTTON;

			if (g_particularMode) {
				if (controller.dualsenseCurInputState.ButtonCreate) bitmaskButtons |= SCE_BM_SHARE;
				if (controller.dualsenseCurInputState.ButtonHome) bitmaskButtons |= SCE_BM_PSBTN;
			}

			data->bitmask_buttons = bitmaskButtons;
		#pragma endregion

		#pragma region sticks
			data->LeftStick.X = controller.dualsenseCurInputState.LeftStickX;
			data->LeftStick.Y = controller.dualsenseCurInputState.LeftStickY;
			data->RightStick.X = controller.dualsenseCurInputState.RightStickX;
			data->RightStick.Y = controller.dualsenseCurInputState.RightStickY;
		#pragma endregion

		#pragma region triggers
			data->L2_Analog = controller.dualsenseCurInputState.TriggerLeft;
			data->R2_Analog = controller.dualsenseCurInputState.TriggerRight;
		#pragma endregion

		#pragma region gyro		
			if (controller.motionSensorState) {
				auto now = std::chrono::steady_clock::now();
				controller.deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(now - controller.lastUpdate).count() / 1000000.0f;
				controller.lastUpdate = now;

				data->acceleration.x = (float)controller.dualsenseCurInputState.AccelerometerX;
				data->acceleration.y = (float)controller.dualsenseCurInputState.AccelerometerY;
				data->acceleration.z = (float)controller.dualsenseCurInputState.AccelerometerZ;

				data->angularVelocity.x = (float)controller.dualsenseCurInputState.AngularVelocityX;
				data->angularVelocity.y = (float)controller.dualsenseCurInputState.AngularVelocityY;
				data->angularVelocity.z = (float)controller.dualsenseCurInputState.AngularVelocityZ;

				data->angularVelocity.x = controller.velocityDeadband == true && (data->angularVelocity.x < ANGULAR_VELOCITY_DEADBAND_MIN && data->angularVelocity.x > -ANGULAR_VELOCITY_DEADBAND_MIN) ? 0 : data->angularVelocity.x;
				data->angularVelocity.y = controller.velocityDeadband == true && (data->angularVelocity.y < ANGULAR_VELOCITY_DEADBAND_MIN && data->angularVelocity.y > -ANGULAR_VELOCITY_DEADBAND_MIN) ? 0 : data->angularVelocity.y;
				data->angularVelocity.z = controller.velocityDeadband == true && (data->angularVelocity.z < ANGULAR_VELOCITY_DEADBAND_MIN && data->angularVelocity.z > -ANGULAR_VELOCITY_DEADBAND_MIN) ? 0 : data->angularVelocity.z;

				auto& q = controller.orientation;
				s_SceFQuaternion w = {
					data->angularVelocity.x,
					data->angularVelocity.y,
					data->angularVelocity.z,
					0.0f
				};

				s_SceFQuaternion qw = {
				  q.w * w.x + q.x * w.w + q.y * w.z - q.z * w.y,
				  q.w * w.y + q.y * w.w + q.z * w.x - q.x * w.z,
				  q.w * w.z + q.z * w.w + q.x * w.y - q.y * w.x,
				  q.w * w.w - q.x * w.x - q.y * w.y - q.z * w.z
				};

				s_SceFQuaternion qDot = { 0.5f * qw.x, 0.5f * qw.y, 0.5f * qw.z, 0.5f * qw.w };

				q.x += qDot.x * controller.deltaTime;
				q.y += qDot.y * controller.deltaTime;
				q.z += qDot.z * controller.deltaTime;
				q.w += qDot.w * controller.deltaTime;

				float norm = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
				q.x /= norm; q.y /= norm; q.z /= norm; q.w /= norm;

				data->orientation.x = q.x;
				data->orientation.y = q.z;
				data->orientation.z = q.y; // yes this is swapped on purpose don't touch it
				data->orientation.w = q.w;
			}
		#pragma endregion

		#pragma region touchpad
			data->touchData.touchNum = (controller.dualsenseCurInputState.touchData.Finger[0].NotTouching > 0 ? 0 : 1) + (controller.dualsenseCurInputState.touchData.Finger[1].NotTouching > 0 ? 0 : 1);

			data->touchData.touch[0].id = controller.dualsenseCurInputState.touchData.Finger[0].Index;
			data->touchData.touch[0].x = controller.dualsenseCurInputState.touchData.Finger[0].FingerX;
			data->touchData.touch[0].y = controller.dualsenseCurInputState.touchData.Finger[0].FingerY;
			data->touchData.touch[0].reserve[0] = controller.dualsenseCurInputState.touchData.Finger[0].NotTouching;

			data->touchData.touch[1].id = controller.dualsenseCurInputState.touchData.Finger[1].Index;
			data->touchData.touch[1].x = controller.dualsenseCurInputState.touchData.Finger[1].FingerX;
			data->touchData.touch[1].y = controller.dualsenseCurInputState.touchData.Finger[1].FingerY;
			data->touchData.touch[1].reserve[0] = controller.dualsenseCurInputState.touchData.Finger[1].NotTouching;
		#pragma endregion

		#pragma region misc
			data->connected = controller.valid;
			data->timestamp = controller.dualsenseCurInputState.DeviceTimeStamp;
			data->extUnitData = {};
			data->connectionCount = 0;
			for (int j = 0; j < 12; j++)
				data->deviceUniqueData[j] = {};
			data->deviceUniqueDataLen = sizeof(data->deviceUniqueData);
		#pragma endregion
		}
		else if (controller.deviceType == DUALSHOCK4) {
		#pragma region buttons
			uint32_t bitmaskButtons = 0;
			if (controller.dualshock4CurInputState.ButtonCross) bitmaskButtons |= SCE_BM_CROSS;
			if (controller.dualshock4CurInputState.ButtonCircle) bitmaskButtons |= SCE_BM_CIRCLE;
			if (controller.dualshock4CurInputState.ButtonTriangle) bitmaskButtons |= SCE_BM_TRIANGLE;
			if (controller.dualshock4CurInputState.ButtonSquare) bitmaskButtons |= SCE_BM_SQUARE;

			if (controller.dualshock4CurInputState.ButtonL1) bitmaskButtons |= SCE_BM_L1;
			if (controller.dualshock4CurInputState.ButtonL2) bitmaskButtons |= SCE_BM_L2;
			if (controller.dualshock4CurInputState.ButtonR1) bitmaskButtons |= SCE_BM_R1;
			if (controller.dualshock4CurInputState.ButtonR2) bitmaskButtons |= SCE_BM_R2;

			if (controller.dualshock4CurInputState.ButtonL3) bitmaskButtons |= SCE_BM_L3;
			if (controller.dualshock4CurInputState.ButtonR3) bitmaskButtons |= SCE_BM_R3;

			if (controller.dualshock4CurInputState.DPad == Direction::NorthEast) bitmaskButtons |= SCE_BM_N_DPAD + SCE_BM_E_DPAD;
			if (controller.dualshock4CurInputState.DPad == Direction::NorthWest) bitmaskButtons |= SCE_BM_N_DPAD + SCE_BM_W_DPAD;
			if (controller.dualshock4CurInputState.DPad == Direction::SouthEast) bitmaskButtons |= SCE_BM_S_DPAD + SCE_BM_E_DPAD;
			if (controller.dualshock4CurInputState.DPad == Direction::SouthWest) bitmaskButtons |= SCE_BM_S_DPAD + SCE_BM_W_DPAD;

			if (controller.dualshock4CurInputState.DPad == Direction::North) bitmaskButtons |= SCE_BM_N_DPAD;
			if (controller.dualshock4CurInputState.DPad == Direction::South) bitmaskButtons |= SCE_BM_S_DPAD;
			if (controller.dualshock4CurInputState.DPad == Direction::East) bitmaskButtons |= SCE_BM_E_DPAD;
			if (controller.dualshock4CurInputState.DPad == Direction::West) bitmaskButtons |= SCE_BM_W_DPAD;

			if (controller.dualshock4CurInputState.ButtonOptions) bitmaskButtons |= SCE_BM_OPTIONS;

			if (controller.dualshock4CurInputState.ButtonPad) bitmaskButtons |= SCE_BM_TOUCH;

			if (g_particularMode) {
				if (controller.dualshock4CurInputState.ButtonShare) bitmaskButtons |= SCE_BM_SHARE;
				if (controller.dualshock4CurInputState.ButtonHome) bitmaskButtons |= SCE_BM_PSBTN;
			}

			data->bitmask_buttons = bitmaskButtons;
		#pragma endregion

		#pragma region sticks
			data->LeftStick.X = controller.dualshock4CurInputState.LeftStickX;
			data->LeftStick.Y = controller.dualshock4CurInputState.LeftStickY;
			data->RightStick.X = controller.dualshock4CurInputState.RightStickX;
			data->RightStick.Y = controller.dualshock4CurInputState.RightStickY;
		#pragma endregion

		#pragma region triggers
			data->L2_Analog = controller.dualshock4CurInputState.TriggerLeft;
			data->R2_Analog = controller.dualshock4CurInputState.TriggerRight;
		#pragma endregion

		#pragma region gyro		
			if (controller.motionSensorState) {
				auto now = std::chrono::steady_clock::now();
				controller.deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(now - controller.lastUpdate).count() / 1000000.0f;
				controller.lastUpdate = now;

				data->acceleration.x = (float)controller.dualshock4CurInputState.AccelerometerX;
				data->acceleration.y = (float)controller.dualshock4CurInputState.AccelerometerY;
				data->acceleration.z = (float)controller.dualshock4CurInputState.AccelerometerZ;

				data->angularVelocity.x = (float)controller.dualshock4CurInputState.AngularVelocityX;
				data->angularVelocity.y = (float)controller.dualshock4CurInputState.AngularVelocityY;
				data->angularVelocity.z = (float)controller.dualshock4CurInputState.AngularVelocityZ;

				data->angularVelocity.x = controller.velocityDeadband == true && (data->angularVelocity.x < ANGULAR_VELOCITY_DEADBAND_MIN && data->angularVelocity.x > -ANGULAR_VELOCITY_DEADBAND_MIN) ? 0 : data->angularVelocity.x;
				data->angularVelocity.y = controller.velocityDeadband == true && (data->angularVelocity.y < ANGULAR_VELOCITY_DEADBAND_MIN && data->angularVelocity.y > -ANGULAR_VELOCITY_DEADBAND_MIN) ? 0 : data->angularVelocity.y;
				data->angularVelocity.z = controller.velocityDeadband == true && (data->angularVelocity.z < ANGULAR_VELOCITY_DEADBAND_MIN && data->angularVelocity.z > -ANGULAR_VELOCITY_DEADBAND_MIN) ? 0 : data->angularVelocity.z;

				auto& q = controller.orientation;
				s_SceFQuaternion w = { data->angularVelocity.x,
						   data->angularVelocity.y,
						   data->angularVelocity.z,
						   0.0f };

				s_SceFQuaternion qw = {
				  q.w * w.x + q.x * w.w + q.y * w.z - q.z * w.y,
				  q.w * w.y + q.y * w.w + q.z * w.x - q.x * w.z,
				  q.w * w.z + q.z * w.w + q.x * w.y - q.y * w.x,
				  q.w * w.w - q.x * w.x - q.y * w.y - q.z * w.z
				};

				s_SceFQuaternion qDot = { 0.5f * qw.x, 0.5f * qw.y, 0.5f * qw.z, 0.5f * qw.w };

				q.x += qDot.x * controller.deltaTime;
				q.y += qDot.y * controller.deltaTime;
				q.z += qDot.z * controller.deltaTime;
				q.w += qDot.w * controller.deltaTime;

				float norm = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
				q.x /= norm; q.y /= norm; q.z /= norm; q.w /= norm;

				data->orientation.x = q.x;
				data->orientation.y = q.z;
				data->orientation.z = q.y;  // yes this is swapped on purpose don't touch it
				data->orientation.w = q.w;
			}
		#pragma endregion

		#pragma region touchpad
			data->touchData.touchNum = (controller.dualshock4CurInputState.Finger1Active > 0 ? 0 : 1) + (controller.dualshock4CurInputState.Finger2Active > 0 ? 0 : 1);

			data->touchData.touch[0].id = controller.dualshock4CurInputState.Finger1ID;
			data->touchData.touch[0].x = controller.dualshock4CurInputState.Finger1X;
			data->touchData.touch[0].y = controller.dualshock4CurInputState.Finger1Y;
			data->touchData.touch[0].reserve[0] = controller.dualshock4CurInputState.Finger1Active;

			data->touchData.touch[1].id = controller.dualshock4CurInputState.Finger2ID;
			data->touchData.touch[1].x = controller.dualshock4CurInputState.Finger2X;
			data->touchData.touch[1].y = controller.dualshock4CurInputState.Finger2Y;
			data->touchData.touch[1].reserve[0] = controller.dualshock4CurInputState.Finger2Active;
		#pragma endregion

		#pragma region misc
			data->connected = controller.valid;
			data->timestamp = controller.dualshock4CurInputState.Timestamp;
			data->extUnitData = {};
			data->connectionCount = 0;
			for (int j = 0; j < 12; j++)
				data->deviceUniqueData[j] = {};
			data->deviceUniqueDataLen = sizeof(data->deviceUniqueData);
		#pragma endregion

		}

		return SCE_OK;
	}

	return SCE_PAD_ERROR_INVALID_HANDLE;
}

int scePadGetContainerIdInformation(int handle, s_ScePadContainerIdInfo* containerIdInfo) {
	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;
	if (!containerIdInfo) return SCE_PAD_ERROR_INVALID_ARG;

#if defined(_WIN32) || defined(_WIN64) // Windows only for now
	for (auto& controller : g_controllers) {
		std::shared_lock guard(controller.lock);
		if (controller.sceHandle == handle && controller.id != "" && controller.idSize != 0) {
			if (!controller.valid) return SCE_PAD_ERROR_DEVICE_NOT_CONNECTED;

			s_ScePadContainerIdInfo info = {};
			info.size = controller.idSize;
			strncpy_s(info.id, controller.id.c_str(), sizeof(info.id) - 1);
			info.id[sizeof(info.id) - 1] = '\0';
			*containerIdInfo = info;
			return SCE_OK;
		}
	}
	containerIdInfo->size = 0;
	containerIdInfo->id[0] = '\0';
	return SCE_OK;
#else
	return SCE_PAD_ERROR_NOT_PERMITTED;
#endif
}

int scePadSetLightBar(int handle, s_SceLightBar* lightbar) {
	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;

	for (auto& controller : g_controllers) {
		std::shared_lock guard(controller.lock);

		if (controller.sceHandle != handle) continue;
		if (!controller.valid) return SCE_PAD_ERROR_DEVICE_NOT_CONNECTED;

		if (controller.deviceType == DUALSENSE) {
			controller.dualsenseCurOutputState.LedRed = lightbar->r;
			controller.dualsenseCurOutputState.LedGreen = lightbar->g;
			controller.dualsenseCurOutputState.LedBlue = lightbar->b;
		}
		else if (controller.deviceType == DUALSHOCK4) {
			controller.dualshock4CurOutputState.LedRed = lightbar->r;
			controller.dualshock4CurOutputState.LedGreen = lightbar->g;
			controller.dualshock4CurOutputState.LedBlue = lightbar->b;
		}

		return SCE_OK;
	}

	return SCE_PAD_ERROR_INVALID_HANDLE;
}

int scePadGetHandle(int userID, int unk1, int unk2) {
	volatile int preventOptim1 = unk1;
	volatile int preventOptim2 = unk2;
	(void)preventOptim1;
	(void)preventOptim2;

	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;
	if (userID > MAX_CONTROLLER_COUNT || userID < 1) return SCE_PAD_ERROR_INVALID_PORT;

	for (int i = 0; i < MAX_CONTROLLER_COUNT - 1; i++) {
		std::shared_lock guard(g_controllers[i].lock);

		if (g_controllers[i].playerIndex != userID) continue;
		return g_controllers[i].sceHandle;
	}

	return SCE_PAD_ERROR_NO_HANDLE;
}

int scePadResetLightBar(int handle) {
	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;

	for (auto& controller : g_controllers) {
		std::shared_lock guard(controller.lock);

		if (controller.sceHandle != handle) continue;
		if (!controller.valid) return SCE_PAD_ERROR_DEVICE_NOT_CONNECTED;

		if (controller.deviceType == DUALSENSE) {
			controller.dualsenseCurOutputState.LedRed = 0;
			controller.dualsenseCurOutputState.LedGreen = 0;
			controller.dualsenseCurOutputState.LedBlue = 0;
		}
		else if (controller.deviceType == DUALSHOCK4) {
			controller.dualshock4CurOutputState.LedRed = 0;
			controller.dualshock4CurOutputState.LedGreen = 0;
			controller.dualshock4CurOutputState.LedBlue = 0;
		}
		return SCE_OK;
	}
	return SCE_PAD_ERROR_INVALID_HANDLE;
}

int scePadSetTriggerEffect(int handle, ScePadTriggerEffectParam* triggerEffect) {
	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;

	for (auto& controller : g_controllers) {
		std::shared_lock guard(controller.lock);

		if (controller.sceHandle != handle) continue;
		if (!controller.valid) return SCE_PAD_ERROR_DEVICE_NOT_CONNECTED;
		if (controller.deviceType != DUALSENSE) return SCE_PAD_ERROR_NOT_PERMITTED;

		controller.triggerMask = triggerEffect->triggerMask;

		for (int i = 0; i <= 1; i++) {
			duaLibUtils::trigger _trigger = {};

			if (triggerEffect->command[i].mode == ScePadTriggerEffectMode::SCE_PAD_TRIGGER_EFFECT_MODE_OFF) {
				TriggerEffectGenerator::Off(_trigger.force, 0);
			}
			else if (triggerEffect->command[i].mode == ScePadTriggerEffectMode::SCE_PAD_TRIGGER_EFFECT_MODE_FEEDBACK) {
				TriggerEffectGenerator::Feedback(_trigger.force, 0, triggerEffect->command[i].commandData.feedbackParam.position, triggerEffect->command[i].commandData.feedbackParam.strength);
			}
			else if (triggerEffect->command[i].mode == ScePadTriggerEffectMode::SCE_PAD_TRIGGER_EFFECT_MODE_WEAPON) {
				TriggerEffectGenerator::Weapon(_trigger.force, 0, triggerEffect->command[i].commandData.weaponParam.startPosition, triggerEffect->command[i].commandData.weaponParam.endPosition, triggerEffect->command[i].commandData.weaponParam.strength);
			}
			else if (triggerEffect->command[i].mode == ScePadTriggerEffectMode::SCE_PAD_TRIGGER_EFFECT_MODE_VIBRATION) {
				TriggerEffectGenerator::Vibration(_trigger.force, 0, triggerEffect->command[i].commandData.vibrationParam.position, triggerEffect->command[i].commandData.vibrationParam.amplitude, triggerEffect->command[i].commandData.vibrationParam.frequency);
			}
			else if (triggerEffect->command[i].mode == ScePadTriggerEffectMode::SCE_PAD_TRIGGER_EFFECT_MODE_SLOPE_FEEDBACK) {
				TriggerEffectGenerator::SlopeFeedback(_trigger.force, 0, triggerEffect->command[i].commandData.slopeFeedbackParam.startPosition, triggerEffect->command[i].commandData.slopeFeedbackParam.endPosition, triggerEffect->command[i].commandData.slopeFeedbackParam.startStrength, triggerEffect->command[i].commandData.slopeFeedbackParam.endStrength);
			}
			else if (triggerEffect->command[i].mode == ScePadTriggerEffectMode::SCE_PAD_TRIGGER_EFFECT_MODE_MULTIPLE_POSITION_FEEDBACK) {
				TriggerEffectGenerator::MultiplePositionFeedback(_trigger.force, 0, triggerEffect->command[i].commandData.multiplePositionFeedbackParam.strength);
			}
			else if (triggerEffect->command[i].mode == ScePadTriggerEffectMode::SCE_PAD_TRIGGER_EFFECT_MODE_MULTIPLE_POSITION_VIBRATION) {
				TriggerEffectGenerator::MultiplePositionVibration(_trigger.force, 0, triggerEffect->command[i].commandData.multiplePositionVibrationParam.frequency, triggerEffect->command[i].commandData.multiplePositionVibrationParam.amplitude);
			}

			if (i == SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2 && controller.triggerMask & SCE_PAD_TRIGGER_EFFECT_TRIGGER_MASK_L2) {
				for (int i = 0; i < 11; i++) {
					controller.L2.force[i] = _trigger.force[i];
				}
			}
			else if (i == SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_R2 && controller.triggerMask & SCE_PAD_TRIGGER_EFFECT_TRIGGER_MASK_R2) {
				for (int i = 0; i < 11; i++) {
					controller.R2.force[i] = _trigger.force[i];
				}
			}
		}

		return SCE_OK;
	}
	return SCE_PAD_ERROR_INVALID_HANDLE;
}

int scePadGetControllerBusType(int handle, int* busType) {
	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;

	for (auto& controller : g_controllers) {
		std::shared_lock guard(controller.lock);

		if (controller.sceHandle != handle) continue;
		if (!controller.valid) return SCE_PAD_ERROR_DEVICE_NOT_CONNECTED;

		*busType = controller.connectionType;

		return SCE_OK;
	}
	return SCE_PAD_ERROR_INVALID_HANDLE;
}

int scePadGetControllerInformation(int handle, s_ScePadInfo* info) {
	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;

	for (auto& controller : g_controllers) {
		std::shared_lock guard(controller.lock);

		if (controller.sceHandle != handle) continue;
		if (!controller.valid) return SCE_PAD_ERROR_DEVICE_NOT_CONNECTED;

		s_ScePadInfo _info = {};

		_info.touchPadInfo.resolution.x = controller.deviceType == DUALSENSE ? 1920 : 1920;
		_info.touchPadInfo.resolution.y = controller.deviceType == DUALSENSE ? 1080 : 943;
		_info.touchPadInfo.pixelDensity = controller.deviceType == DUALSENSE ? 44.86 : 44;
		_info.stickInfo.deadZoneLeft = controller.deviceType == DUALSENSE ? 13 : 13;
		_info.stickInfo.deadZoneRight = controller.deviceType == DUALSENSE ? 13 : 13;
		_info.connectionType = SCE_PAD_CONNECTION_TYPE_LOCAL;
		_info.connectedCount = 1;
		_info.connected = controller.valid;
		_info.deviceClass = SCE_PAD_DEVICE_CLASS_STANDARD;

		*info = _info;
		return SCE_OK;
	}
	return SCE_PAD_ERROR_INVALID_HANDLE;
}

int scePadGetControllerType(int handle, s_SceControllerType* controllerType) {
	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;

	for (auto& controller : g_controllers) {
		std::shared_lock guard(controller.lock);

		if (controller.sceHandle != handle) continue;
		if (!controller.valid) return SCE_PAD_ERROR_DEVICE_NOT_CONNECTED;

		*controllerType = (s_SceControllerType)controller.deviceType;

		return SCE_OK;
	}
	return SCE_PAD_ERROR_INVALID_HANDLE;
}

int scePadGetJackState(int handle, int* state) {
	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;

	for (auto& controller : g_controllers) {
		std::shared_lock guard(controller.lock);

		if (controller.sceHandle != handle) continue;
		if (!controller.valid) return SCE_PAD_ERROR_DEVICE_NOT_CONNECTED;

		if (controller.deviceType == DUALSENSE) {
			*state = controller.dualsenseCurInputState.PluggedHeadphones + controller.dualsenseCurInputState.PluggedMic;
		}
		else if (controller.deviceType == DUALSHOCK4) {
			*state = controller.dualshock4CurInputState.PluggedHeadphones + controller.dualshock4CurInputState.PluggedMic;
		}

		return SCE_OK;
	}
	return SCE_PAD_ERROR_INVALID_HANDLE;
}

int scePadGetTriggerEffectState(int handle, int state[2]) {
	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;

	for (auto& controller : g_controllers) {
		std::shared_lock guard(controller.lock);

		if (controller.sceHandle != handle) continue;
		if (!controller.valid) return SCE_PAD_ERROR_DEVICE_NOT_CONNECTED;
		if (controller.deviceType != DUALSENSE) return SCE_PAD_ERROR_NOT_PERMITTED;

		switch (controller.dualsenseCurInputState.TriggerLeftEffect) {
			case 1:
				switch (controller.dualsenseCurInputState.TriggerLeftStatus) {
					case 0:
						state[0] = SCE_PAD_TRIGGER_STATE_FEEDBACK_NO_FORCE;
						break;
					case 1:
						state[0] = SCE_PAD_TRIGGER_STATE_FEEDBACK_IS_PUSHING;
						break;
				}
				break;
			case 2:
				switch (controller.dualsenseCurInputState.TriggerLeftStatus) {
					case 0:
						state[0] = SCE_PAD_TRIGGER_STATE_WEAPON_NOT_PRESSED;
						break;
					case 1:
						state[0] = SCE_PAD_TRIGGER_STATE_WEAPON_ALMOST_PRESSED;
						break;
					case 2:
						state[0] = SCE_PAD_TRIGGER_STATE_WEAPON_FULLY_PRESSED;
						break;
				}
				break;
			case 3:
				switch (controller.dualsenseCurInputState.TriggerLeftStatus) {
					case 0:
						state[0] = SCE_PAD_TRIGGER_STATE_VIBRATION_NOT_FIRING;
						break;
					case 1:
						state[0] = SCE_PAD_TRIGGER_STATE_VIBRATION_IS_FIRING;
						break;
				}
				break;
		}

		switch (controller.dualsenseCurInputState.TriggerRightEffect) {
			case 1:
				switch (controller.dualsenseCurInputState.TriggerRightStatus) {
					case 0:
						state[1] = SCE_PAD_TRIGGER_STATE_FEEDBACK_NO_FORCE;
						break;
					case 1:
						state[1] = SCE_PAD_TRIGGER_STATE_FEEDBACK_IS_PUSHING;
						break;
				}
				break;
			case 2:
				switch (controller.dualsenseCurInputState.TriggerRightStatus) {
					case 0:
						state[1] = SCE_PAD_TRIGGER_STATE_WEAPON_NOT_PRESSED;
						break;
					case 1:
						state[1] = SCE_PAD_TRIGGER_STATE_WEAPON_ALMOST_PRESSED;
						break;
					case 2:
						state[1] = SCE_PAD_TRIGGER_STATE_WEAPON_FULLY_PRESSED;
						break;
				}
				break;
			case 3:
				switch (controller.dualsenseCurInputState.TriggerRightStatus) {
					case 0:
						state[1] = SCE_PAD_TRIGGER_STATE_VIBRATION_NOT_FIRING;
						break;
					case 1:
						state[1] = SCE_PAD_TRIGGER_STATE_VIBRATION_IS_FIRING;
						break;
				}
				break;
		}


		return SCE_OK;
	}
	return SCE_PAD_ERROR_INVALID_HANDLE;
}

int scePadIsControllerUpdateRequired(int handle) {
	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;

	for (auto& controller : g_controllers) {
		std::shared_lock guard(controller.lock);

		if (controller.sceHandle != handle) continue;
		if (!controller.valid) return SCE_PAD_ERROR_DEVICE_NOT_CONNECTED;
		if (controller.productID != DUALSENSE_DEVICE_ID && controller.productID != DUALSENSE_EDGE_DEVICE_ID) return -2137915385LL; // undocumented error

		if (controller.productID == DUALSENSE_DEVICE_ID && controller.versionReport.UpdateVersion < 0x390u) {
			return SCE_PAD_UPDATE_REQUIRED;
		}

		if (controller.productID == DUALSENSE_EDGE_DEVICE_ID && controller.versionReport.UpdateVersion < 0x150u) {
			return SCE_PAD_UPDATE_REQUIRED;
		}

		return SCE_PAD_UPDATE_NOT_REQUIRED;
	}
	return SCE_PAD_ERROR_INVALID_HANDLE;
}

int scePadRead(int handle, s_ScePadData* data, int count) {
	// No idea what's the purpose of this, in the original library it does literally the same thing as scePadReadState but the program crashes when count is bigger than 20

	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;
	if ((count - 1) > 63) return SCE_PAD_ERROR_INVALID_ARG;

	s_ScePadData thisData = {};
	int res = scePadReadState(handle, &thisData);
	std::memcpy(data, &thisData, sizeof(thisData));
	return res;
}

int scePadResetOrientation(int handle) {
	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;

	for (auto& controller : g_controllers) {
		std::shared_lock guard(controller.lock);

		if (controller.sceHandle != handle) continue;
		if (!controller.valid) return SCE_PAD_ERROR_DEVICE_NOT_CONNECTED;

		controller.orientation = { 0.0f,0.0f,0.0f,1.0f };

		return SCE_OK;
	}

	return SCE_PAD_ERROR_INVALID_HANDLE;
}

int scePadSetAngularVelocityDeadbandState(int handle, bool state) {
	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;

	for (auto& controller : g_controllers) {
		std::shared_lock guard(controller.lock);

		if (controller.sceHandle != handle) continue;
		if (!controller.valid) return SCE_PAD_ERROR_DEVICE_NOT_CONNECTED;

		controller.velocityDeadband = state;

		return SCE_OK;
	}

	return SCE_PAD_ERROR_INVALID_HANDLE;
}

int scePadSetAudioOutPath(int handle, int path) {
	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;
	if (path > 4) return SCE_PAD_ERROR_INVALID_ARG;

	for (auto& controller : g_controllers) {
		std::shared_lock guard(controller.lock);

		if (controller.sceHandle != handle) continue;
		if (!controller.valid) return SCE_PAD_ERROR_DEVICE_NOT_CONNECTED;

		if (controller.deviceType == DUALSENSE) {
			controller.dualsenseCurOutputState.OutputPathSelect = path;
		}
		else if (controller.deviceType == DUALSHOCK4) {
			controller.dualshock4CurAudio.Output = (dualshock4Data::AudioOutput)path;
		}

		return SCE_OK;
	}

	return SCE_PAD_ERROR_INVALID_HANDLE;
}

int scePadSetMotionSensorState(int handle, bool state) {
	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;

	for (auto& controller : g_controllers) {
		std::shared_lock guard(controller.lock);

		if (controller.sceHandle != handle) continue;
		if (!controller.valid) { return SCE_PAD_ERROR_DEVICE_NOT_CONNECTED; }

		controller.motionSensorState = state;

		return SCE_OK;
	}

	return SCE_PAD_ERROR_INVALID_HANDLE;
}

int scePadSetTiltCorrectionState(int handle, bool state) {
	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;

	for (auto& controller : g_controllers) {
		std::shared_lock guard(controller.lock);

		if (controller.sceHandle != handle) continue;
		if (!controller.valid) return SCE_PAD_ERROR_DEVICE_NOT_CONNECTED;

		controller.tiltCorrection = state;

		return SCE_OK;
	}

	return SCE_PAD_ERROR_INVALID_HANDLE;
}

int scePadSetVibration(int handle, s_ScePadVibrationParam* vibration) {
	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;

	for (auto& controller : g_controllers) {
		std::shared_lock guard(controller.lock);

		if (controller.sceHandle != handle) continue;
		if (!controller.valid) return SCE_PAD_ERROR_DEVICE_NOT_CONNECTED;

		if (controller.deviceType == DUALSENSE) {
			controller.dualsenseCurOutputState.RumbleEmulationLeft = vibration->largeMotor;
			controller.dualsenseCurOutputState.RumbleEmulationRight = vibration->smallMotor;
		}
		else if (controller.deviceType == DUALSHOCK4) {
			controller.dualshock4CurOutputState.RumbleLeft = vibration->largeMotor;
			controller.dualshock4CurOutputState.RumbleRight = vibration->smallMotor;
		}

		return SCE_OK;
	}

	return SCE_PAD_ERROR_INVALID_HANDLE;
}

int scePadSetVibrationMode(int handle, int mode) {
	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;
	if (mode <= 0) return SCE_PAD_ERROR_INVALID_ARG;

	for (auto& controller : g_controllers) {
		std::shared_lock guard(controller.lock);

		if (controller.sceHandle != handle) continue;
		if (!controller.valid) return SCE_PAD_ERROR_DEVICE_NOT_CONNECTED;

		if (controller.deviceType == DUALSENSE) {
			if (mode == SCE_PAD_HAPTICS_MODE) {
				controller.dualsenseCurOutputState.UseRumbleNotHaptics = false;
				controller.dualsenseCurOutputState.EnableRumbleEmulation = false;
				controller.dualsenseCurOutputState.EnableImprovedRumbleEmulation = false;
			}
			else if (mode == SCE_PAD_RUMBLE_MODE) {
				controller.dualsenseCurOutputState.UseRumbleNotHaptics = true;

				if (controller.versionReport.FirmwareVersion >= 0x220) {
					controller.dualsenseCurOutputState.EnableImprovedRumbleEmulation = true;
				}
				else {
					controller.dualsenseCurOutputState.EnableRumbleEmulation = true;
				}
			}
		}

		return SCE_OK;
	}
	return SCE_PAD_ERROR_INVALID_HANDLE;
}

int scePadSetVolumeGain(int handle, s_ScePadVolumeGain* gainSettings) {
	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;
	if (!gainSettings || ((gainSettings->speakerVolume + 128) <= 126 || (gainSettings->micGain + 128) <= 126 || (gainSettings->headsetVolume + 128) <= 126)) return SCE_PAD_ERROR_INVALID_ARG;

	for (auto& controller : g_controllers) {
		std::shared_lock guard(controller.lock);

		if (controller.sceHandle != handle) continue;
		if (!controller.valid) return SCE_PAD_ERROR_DEVICE_NOT_CONNECTED;

		if (controller.deviceType == DUALSENSE) {
			controller.dualsenseCurOutputState.VolumeSpeaker = gainSettings->speakerVolume + 64;
			controller.dualsenseCurOutputState.VolumeMic = gainSettings->micGain;
			controller.dualsenseCurOutputState.VolumeHeadphones = gainSettings->headsetVolume + 64;
		}
		else if (controller.deviceType == DUALSHOCK4) {
			controller.dualshock4CurOutputState.VolumeSpeaker = 40 + (int)((gainSettings->speakerVolume / 126.0) * 79);
			controller.dualshock4CurOutputState.VolumeMic = 40 + (int)((gainSettings->micGain / 100.0) * 79);
			controller.dualshock4CurOutputState.VolumeLeft = 40 + (int)((gainSettings->headsetVolume / 100.0) * 79);
			controller.dualshock4CurOutputState.VolumeRight = 40 + (int)((gainSettings->headsetVolume / 100.0) * 79);
		}

		return SCE_OK;
	}
	return SCE_PAD_ERROR_INVALID_HANDLE;
}

int scePadIsSupportedAudioFunction(int handle) {
	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;

	for (auto& controller : g_controllers) {
		std::shared_lock guard(controller.lock);

		if (controller.sceHandle != handle) continue;
		if (!controller.valid) return SCE_PAD_ERROR_DEVICE_NOT_CONNECTED;

		// The original function doesn't include DualShock 4 v1 for some reason.
		if (controller.productID == DUALSHOCK4_DEVICE_ID || controller.productID == DUALSHOCK4V2_DEVICE_ID || controller.productID == DUALSHOCK4_WIRELESS_ADAPTOR_ID || controller.productID == DUALSENSE_DEVICE_ID || controller.productID == DUALSENSE_EDGE_DEVICE_ID) {
			return 1;
		}

		return SCE_OK;
	}
	return SCE_PAD_ERROR_INVALID_HANDLE;
}

int scePadClose(int handle) {
	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;

	for (auto& controller : g_controllers) {
		std::shared_lock guard(controller.lock);

		if (controller.sceHandle != handle) continue;
		if (!controller.valid) return SCE_PAD_ERROR_DEVICE_NOT_CONNECTED;

		controller.opened = false;
		controller.valid = false;
		controller.sceHandle = 0;
		controller.lastPath = "";
		controller.productID = 0;
		controller.wasDisconnected = true;
		controller.macAddress = "";
		duaLibUtils::letGo(controller.handle, controller.deviceType, controller.connectionType);

		return SCE_OK;
	}
	return SCE_PAD_ERROR_INVALID_HANDLE;
}

int scePadSetPlayerLedBrightness(int handle, int brightness) {
	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;

	for (auto& controller : g_controllers) {
		std::shared_lock guard(controller.lock);

		if (controller.sceHandle != handle) continue;
		if (!controller.valid) return SCE_PAD_ERROR_DEVICE_NOT_CONNECTED;

		controller.dualsenseCurOutputState.lightBrightness = (dualsenseData::LightBrightness)brightness;

		return SCE_OK;
	}
	return SCE_PAD_ERROR_INVALID_HANDLE;
}

int scePadSetPlayerLed(int handle, bool state) {
	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;

	for (auto& controller : g_controllers) {
		std::shared_lock guard(controller.lock);

		if (controller.sceHandle != handle) continue;
		if (!controller.valid) return SCE_PAD_ERROR_DEVICE_NOT_CONNECTED;

		controller.playerLed = state;

		return SCE_OK;
	}
	return SCE_PAD_ERROR_INVALID_HANDLE;
}

std::string scePadGetMacAddress(int handle) {
	if (!g_initialized) return "";

	for (auto& controller : g_controllers) {
		std::shared_lock guard(controller.lock);

		if (controller.sceHandle != handle) continue;
		if (!controller.valid) return "";

		return controller.macAddress;
	}
	return "";
}

std::string scePadGetPath(int handle) {
	if (!g_initialized) return "";

	for (auto& controller : g_controllers) {
		std::shared_lock guard(controller.lock);

		if (controller.sceHandle != handle) continue;
		if (!controller.valid) return "";

		return controller.lastPath;
	}
	return "";
}

int scePadSetTriggerEffectCustom(int handle, uint8_t left[11], uint8_t right[11], uint8_t triggerBitmask) {
	if (!g_initialized) return SCE_PAD_ERROR_NOT_INITIALIZED;

	for (auto& controller : g_controllers) {
		std::shared_lock guard(controller.lock);

		if (controller.sceHandle != handle) continue;
		if (!controller.valid) return SCE_PAD_ERROR_DEVICE_NOT_CONNECTED;

		if(left != nullptr && triggerBitmask & SCE_PAD_TRIGGER_EFFECT_TRIGGER_MASK_L2) {
			controller.triggerMask |= SCE_PAD_TRIGGER_EFFECT_TRIGGER_MASK_L2;
			std::memcpy(controller.L2.force, left, sizeof(controller.L2.force));
		}
		if(right != nullptr && triggerBitmask & SCE_PAD_TRIGGER_EFFECT_TRIGGER_MASK_R2) {
			controller.triggerMask |= SCE_PAD_TRIGGER_EFFECT_TRIGGER_MASK_R2;
			std::memcpy(controller.R2.force, right, sizeof(controller.R2.force));
		}

		return SCE_OK;
	}
	return SCE_PAD_ERROR_INVALID_HANDLE;
}

#if COMPILE_TO_EXE
int main() {
	s_ScePadInitParam initParam = {};
	initParam.allowBT = true;
	if (scePadInit3(&initParam) != SCE_OK) {
		std::cout << "Failed to initalize!" << std::endl;
	}

	//int handle = scePadOpen(1, NULL, NULL, NULL);
	int handle = scePadOpen(1, 0, 0);
	int handle2 = scePadOpen(2, 0, 0);
	int handle3 = scePadOpen(3, 0, 0);
	int handle4 = scePadOpen(4, 0, 0);

	std::cout << handle << std::endl;
	//std::cout << handle2 << std::endl;
	getchar();
	s_SceLightBar l = {};
	l.g = 255;
	scePadSetLightBar(handle, &l);
	//scePadSetLightBar(handle2, &l);
	int res = scePadSetAudioOutPath(handle, SCE_PAD_AUDIO_PATH_ONLY_SPEAKER);
	res = scePadSetAudioOutPath(handle2, SCE_PAD_AUDIO_PATH_ONLY_SPEAKER);
	std::cout << res << std::endl;
	//scePadSetAudioOutPath(handle2, SCE_PAD_AUDIO_PATH_ONLY_SPEAKER);;
	ScePadTriggerEffectParam trigger = {};
	trigger.triggerMask = SCE_PAD_TRIGGER_EFFECT_TRIGGER_MASK_L2 | SCE_PAD_TRIGGER_EFFECT_TRIGGER_MASK_R2;
	trigger.command[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2].mode = ScePadTriggerEffectMode::SCE_PAD_TRIGGER_EFFECT_MODE_MULTIPLE_POSITION_VIBRATION;
	trigger.command[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2].commandData.multiplePositionVibrationParam.amplitude[0] = 0;
	trigger.command[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2].commandData.multiplePositionVibrationParam.amplitude[1] = 8;
	trigger.command[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2].commandData.multiplePositionVibrationParam.amplitude[2] = 0;
	trigger.command[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2].commandData.multiplePositionVibrationParam.amplitude[3] = 8;
	trigger.command[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2].commandData.multiplePositionVibrationParam.amplitude[4] = 0;
	trigger.command[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2].commandData.multiplePositionVibrationParam.amplitude[5] = 8;
	trigger.command[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2].commandData.multiplePositionVibrationParam.amplitude[6] = 0;
	trigger.command[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2].commandData.multiplePositionVibrationParam.amplitude[7] = 8;
	trigger.command[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2].commandData.multiplePositionVibrationParam.amplitude[8] = 0;
	trigger.command[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2].commandData.multiplePositionVibrationParam.amplitude[9] = 8;
	trigger.command[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2].commandData.multiplePositionVibrationParam.frequency = 15;

	trigger.command[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_R2].mode = ScePadTriggerEffectMode::SCE_PAD_TRIGGER_EFFECT_MODE_WEAPON;
	trigger.command[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_R2].commandData.weaponParam.startPosition = 5;
	trigger.command[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_R2].commandData.weaponParam.endPosition = 6;
	trigger.command[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_R2].commandData.weaponParam.strength = 8;

	scePadSetTriggerEffect(handle, &trigger);

	s_ScePadInfo info;
	scePadGetControllerInformation(handle, &info);

	s_ScePadVibrationParam vibr = { 255,255 };
	//scePadSetVibration(handle2, &vibr);

	ScePadTriggerEffectParam trigger2 = {};
	trigger2.triggerMask = SCE_PAD_TRIGGER_EFFECT_TRIGGER_MASK_L2;
	trigger2.command[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2].mode = ScePadTriggerEffectMode::SCE_PAD_TRIGGER_EFFECT_MODE_WEAPON;
	trigger2.command[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2].commandData.weaponParam.startPosition = 2;
	trigger2.command[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2].commandData.weaponParam.endPosition = 7;
	trigger2.command[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2].commandData.weaponParam.strength = 7;

	scePadSetAngularVelocityDeadbandState(handle, true);
	//scePadSetAngularVelocityDeadbandState(handle2, false);
	scePadSetMotionSensorState(handle, true);

	scePadSetAudioOutPath(handle2, SCE_PAD_AUDIO_PATH_ONLY_SPEAKER);
	scePadSetVibrationMode(handle, SCE_PAD_RUMBLE_MODE);

	s_ScePadVolumeGain volume = {};
	volume.speakerVolume = 100;
	volume.micGain = 64;
	scePadSetVolumeGain(handle, &volume);

	getchar();

	s_ScePadContainerIdInfo cont = {};
	scePadGetContainerIdInformation(handle, &cont);
	std::cout << cont.id << std::endl;

	scePadTerminate();
	return 0;
}
#endif