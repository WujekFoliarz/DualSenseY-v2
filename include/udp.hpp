#ifndef UDP_H
#define UDP_H

#include <asio.hpp>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

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
	uint32_t Index;
	std::string MacAddress;
	DeviceType DeviceType;
	ConnectionType ConnectionType;
	uint32_t BatteryLevel;
	bool IsSupportAT;
	bool IsSupportLightBar;
	bool IsSupportPlayerLED;
	bool IsSupportMicLED;

	nlohmann::json to_json() {
		nlohmann::json j = nlohmann::json{
			{"Index", Index},
			{"MacAddress", MacAddress},
			{"DeviceType", (uint32_t)DeviceType},
			{"ConnectionType", (uint32_t)ConnectionType},
			{"BatteryLevel", BatteryLevel},
			{"IsSupportAT", IsSupportAT},
			{"IsSupportLightBar", IsSupportLightBar},
			{"IsSupportPlayerLED", IsSupportPlayerLED},
			{"IsSupportMicLED", IsSupportMicLED},
		};

		return j;
	}
};

class ServerResponse {
public:
	std::string Status;
	std::string TimeReceived;
	bool isControllerConnected;
	uint32_t BatteryLevel;
	std::vector<Device> Devices;

	nlohmann::json to_json() {
		nlohmann::json deviceArray = nlohmann::json::array();
		for (auto& device : Devices) {
			deviceArray.push_back(device.to_json());
		}

		nlohmann::json j = nlohmann::json {
			{"Status", Status},
			{"TimeReceived", TimeReceived},
			{"isControllerConnected", isControllerConnected},
			{"BatteryLevel", BatteryLevel},
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
			parameters.push_back(parameter);
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
	static void listen(std::atomic<bool>& threadRunning, asio::ip::udp::socket& socket, std::chrono::steady_clock::time_point& lastUpdate);
public:
	bool isActive();
	UDP();
	~UDP();
};

#endif