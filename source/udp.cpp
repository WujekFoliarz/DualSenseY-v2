#include "udp.hpp"
#include "log.hpp"
#include <duaLib.h>
#include <iomanip>

std::string getFormattedDateTime() {
	auto now = std::chrono::system_clock::now();
	std::time_t now_time = std::chrono::system_clock::to_time_t(now);

	std::tm local_tm{};
#if !defined(__linux__) && !defined(__APPLE__)
	localtime_s(&local_tm, &now_time); 
#else
	localtime_r(&now_time, &local_tm);  
#endif

	std::ostringstream oss;
	oss << std::put_time(&local_tm, "%Y/%m/%d %I:%M:%S %p"); 
	return oss.str();
}

void UDP::listen(std::atomic<bool>& threadRunning, asio::ip::udp::socket& socket, std::chrono::steady_clock::time_point& lastUpdate) {
	int scePad[4] = { scePadGetHandle(1, 0, 0) ,scePadGetHandle(2, 0, 0) ,scePadGetHandle(3, 0, 0) ,scePadGetHandle(4, 0, 0) };

	while (threadRunning) {
		char buffer[1024] = {};
		asio::ip::udp::endpoint senderEndpoint;
		try {
			size_t length = socket.receive_from(asio::buffer(buffer), senderEndpoint);
			LOGI("[UDP] Received packet with length %d", length);

			nlohmann::json packetJson = nlohmann::json::parse(buffer);
			Packet packet = {};
			packet.from_json(packetJson);
			
			for (auto& instr : packet.instructions) {
				LOGI("[UDP] Instruction type: %d", instr.type);
			}

			lastUpdate = std::chrono::steady_clock::now();

			ServerResponse response = {};
			response.status = "DSX Received UDP Instructions";
			response.timeReceived = getFormattedDateTime();
			response.batteryLevel = 100;

			for (uint32_t i = 0; i < 4; i++) {				
				s_SceControllerType controllerType = {};
				s_ScePadInfo scePadInfo = {};
				int busType = 0;

				scePadGetControllerType(scePad[i], &controllerType);
				scePadGetControllerInformation(scePad[i], &scePadInfo);
				uint32_t result = scePadGetControllerBusType(scePad[i], &busType);

				if (result == SCE_OK) {
					response.isControllerConnected = true;

					Device device = {};
					device.index = i+1;
					device.macAddress = scePadGetMacAddress(scePad[i]);
					device.deviceType = controllerType == s_SceControllerType::DUALSENSE ? DeviceType::DUALSENSE : DeviceType::DUALSHOCK_V2;	
					device.connectionType = (ConnectionType)(busType-1);
					device.batteryLevel = 100;
					device.isSupportAT = controllerType == s_SceControllerType::DUALSENSE ? true : false;
					device.isSupportLightBar = true;
					device.isSupportPlayerLED = controllerType == s_SceControllerType::DUALSENSE ? true : false;
					device.isSupportMicLED = controllerType == s_SceControllerType::DUALSENSE ? true : false;

					response.devices.push_back(device);
				}
			}

			socket.send_to(asio::buffer(response.to_json().dump(4)), senderEndpoint);
		}
		catch (std::exception& e) {
			LOGE("[UDP] %s", e.what());
			break;
		}
	}
}

bool UDP::isActive() {
	if (m_socket.is_open() && 
	   (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - m_lastUpdate) <= std::chrono::seconds(15))) {
		return true;
	}

	return false;
}

UDP::UDP() : m_socket(m_ioContext, asio::ip::udp::endpoint(asio::ip::udp::v4(), 6969)) {
	if (m_socket.is_open()) {
		m_listenThread = std::thread(listen, std::ref(m_threadRunning), std::ref(m_socket), std::ref(m_lastUpdate));
		m_listenThread.detach();
		LOGI("[UDP] Started");
	}
	else {
		LOGE("[UDP] Failed to start");
	}
}

UDP::~UDP() {
	m_threadRunning = false;
	asio::error_code ec;
	m_socket.close(ec); 
	m_ioContext.stop();  

	m_threadRunning = false;
	if (m_listenThread.joinable()) {
		m_listenThread.join();
	}
}
