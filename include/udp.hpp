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
				parameters.push_back(parameter.get<std::string>());
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

class UDP {
private:
	asio::io_context m_ioContext;
	asio::ip::udp::socket m_socket;
	std::atomic<bool> m_threadRunning = true;
	std::thread m_listenThread;
	std::chrono::steady_clock::time_point m_lastUpdate;
	std::mutex m_settingsLock;
	s_scePadSettings m_settings = {};
	void listen();

	void handleRgbUpdate(Instruction instruction);
public:
	bool isActive();
	s_scePadSettings getSettings();
	UDP();
	~UDP();
};

#endif