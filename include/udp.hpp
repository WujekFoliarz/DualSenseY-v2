#ifndef UDP_H
#define UDP_H

#include <asio.hpp>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <string>
#include <mutex>
#include <nlohmann/json.hpp>
#include "scePadSettings.hpp"

// Server
enum class ConnectionType {
	USB,
	BLUETOOTH,
	DONGLE
};

enum class DeviceType {
	DUALSENSE,
	DUALSENSE_EDGE,
	DUALSHOCK_V1,
	DUALSHOCK_V2,
	DUALSHOCK_DONGLE,
	PS_VR2_LeftController,
	PS_VR2_RightController,
	ACCESS_CONTROLLER
};

class Device {
public:
	uint32_t index;
	std::string macAddress;
	DeviceType deviceType;
	ConnectionType connectionType;
	uint32_t batteryLevel;
	bool isSupportAT;
	bool isSupportLightBar;
	bool isSupportPlayerLED;
	bool isSupportMicLED;

	nlohmann::json to_json() {
		nlohmann::json j = nlohmann::json{
			{"Index", index},
			{"MacAddress", macAddress},
			{"DeviceType", (uint32_t)deviceType},
			{"ConnectionType", (uint32_t)connectionType},
			{"BatteryLevel", batteryLevel},
			{"IsSupportAT", isSupportAT},
			{"IsSupportLightBar", isSupportLightBar},
			{"IsSupportPlayerLED", isSupportPlayerLED},
			{"IsSupportMicLED", isSupportMicLED},
		};

		return j;
	}
};

class ServerResponse {
public:
	std::string status;
	std::string timeReceived;
	bool isControllerConnected;
	uint32_t batteryLevel;
	std::vector<Device> devices;

	nlohmann::json to_json() {
		nlohmann::json deviceArray = nlohmann::json::array();
		for (auto& device : devices) {
			deviceArray.push_back(device.to_json());
		}

		nlohmann::json j = nlohmann::json {
			{"Status", status},
			{"TimeReceived", timeReceived},
			{"isControllerConnected", isControllerConnected},
			{"BatteryLevel", batteryLevel},
			{"Devices", deviceArray},		
		};

		return j;
	}
};

// Client
enum class TriggerMode {
	Normal = 0,
	GameCube = 1,
	VerySoft = 2,
	Soft = 3,
	Hard = 4,
	VeryHard = 5,
	Hardest = 6,
	Rigid = 7,
	VibrateTrigger = 8,
	Choppy = 9,
	Medium = 10,
	VibrateTriggerPulse = 11,
	CustomTriggerValue = 12,
	Resistance = 13,
	Bow = 14,
	Galloping = 15,
	SemiAutomaticGun = 16,
	AutomaticGun = 17,
	Machine = 18,
	VIBRATE_TRIGGER_10Hz = 19,
	OFF = 20,
	FEEDBACK = 21,
	WEAPON = 22,
	VIBRATION = 23,
	SLOPE_FEEDBACK = 24,
	MULTIPLE_POSITION_FEEDBACK = 25,
	MULTIPLE_POSITION_VIBRATION = 26,
};

enum class CustomTriggerValueMode {
	OFF = 0,
	Rigid = 1,
	RigidA = 2,
	RigidB = 3,
	RigidAB = 4,
	Pulse = 5,
	PulseA = 6,
	PulseB = 7,
	PulseAB = 8,
	VibrateResistance = 9,
	VibrateResistanceA = 10,
	VibrateResistanceB = 11,
	VibrateResistanceAB = 12,
	VibratePulse = 13,
	VibratePulseA = 14,
	VibratePulsB = 15,
	VibratePulseAB = 16
};

enum class PlayerLEDNewRevision {
	One = 0,
	Two = 1,
	Three = 2,
	Four = 3,
	Five = 4, // Five is Also All On
	AllOff = 5
};

enum class MicLEDMode {
	On = 0,
	Pulse = 1,
	Off = 2
};

enum class Trigger {
	Invalid,
	Left,
	Right
};

enum class InstructionType {
	GetDSXStatus,
	TriggerUpdate,
	RGBUpdate,
	PlayerLED,
	TriggerThreshold,
	MicLED,
	PlayerLEDNewRevision,
	ResetToUserSettings
};

class Instruction {
public:
	InstructionType type;
	std::vector<std::any> parameters;

	void from_json(nlohmann::json& j) {

		type = (InstructionType)j.at("type").get<int>();

		for (auto& parameter : j["parameters"]) {
			if (parameter.is_number_integer()) {
				parameters.push_back(parameter.get<int>());
			}
			else if (parameter.is_number_float()) {
				parameters.push_back(parameter.get<float>());
			}
			else if (parameter.is_string()) {
				try {
					parameters.push_back(std::stoi(parameter.get<std::string>()));
				}
				catch (...) {
					parameters.push_back(0);
				}
			}
		}
	}
};

class Packet {
public:
	std::vector<Instruction> instructions;

	void from_json(nlohmann::json& j) {
		for (auto& instruction : j["instructions"]) {
			Instruction newInstruction;
			newInstruction.from_json(instruction);
			instructions.push_back(newInstruction);
		}
	}
};

// Maybe swap ASIO with ENet
class UDP {
private:
	asio::io_context m_IoContext;
	asio::ip::udp::socket m_Socket;
	std::atomic<bool> m_ThreadRunning = true;
	std::thread m_ListenThread;
	std::chrono::steady_clock::time_point m_LastUpdate;
	std::mutex m_SettingsLock;
	s_scePadSettings m_Settings = {};
	void Listen();

	void HandleRgbUpdate(Instruction instruction);
	void HandleTriggerUpdate(Instruction instruction);
	void HandleTriggerThresholdUpdate(Instruction instruction);
public:
	bool IsActive();
	s_scePadSettings GetSettings();
	void SetVibrationToUdpConfig(s_ScePadVibrationParam vibration);
	UDP();
	~UDP();
};

#endif