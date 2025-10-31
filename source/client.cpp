#define NOMINMAX
#define ENET_IMPLEMENTATION
#include "client.hpp"
#include "log.hpp"
#include "upnp.hpp"
#include "scePadHandle.hpp"
#include <algorithm>
#include "applicationVersion.hpp"

Client::Client(s_scePadSettings* ScePadSettings) {
	enet_initialize();
	m_LocalIp = GetActiveLocalIP();
	LOGI("Local IP: %s", m_LocalIp.c_str());
	m_PeerControllers = std::make_shared<std::unordered_map<uint32_t, PeerControllerData>>();

	assert(ScePadSettings != nullptr);
	m_ScePadSettings = ScePadSettings;
}

Client::~Client() {
	enet_deinitialize();
	m_ThreadRunning = false;

	if (m_ServiceThread.joinable()) {
		m_ServiceThread.join();
	}

	if (m_InputStateSendoutThread.joinable()) {
		m_InputStateSendoutThread.join();
	}
}

void Client::Connect() {
	m_Connecting = true;
	m_AwaitingResponseCount++;
	if (IsConnected())
		return;

	if (!m_Host) {
		ENetAddress hostAddress = { 0 };
		hostAddress.host = ENET_HOST_ANY;
		hostAddress.port = ENET_PORT_ANY;
		m_Host = enet_host_create(&hostAddress, 100, CHANNEL_COUNT, 0, 0);
	}

	// Failed to create host
	if (!m_Host) {
		m_ConnectionOccupied = true;
		m_AwaitingResponseCount--;
		return;
	}

	ENetAddress serverAddress = { 0 };
	enet_address_set_host(&serverAddress, g_SERVER_HOSTNAME.c_str());
	serverAddress.port = g_PORT;
	m_ServerPeer = enet_host_connect(m_Host, &serverAddress, 2, 0);

	m_AwaitingResponseCount--;
}

void Client::Start() {
	if (m_ThreadRunning)
		return;

	m_ThreadRunning = true;

	m_ServiceThread = std::thread(&Client::HostService, this);
	m_ServiceThread.detach();

	m_InputStateSendoutThread = std::thread(&Client::InputStateSendoutService, this);
	m_InputStateSendoutThread.detach();
}

bool Client::IsConnected() {
	return m_Connected;
}

bool Client::IsConnecting() {
	return !m_Connected && m_Connecting;
}

bool Client::IsFetchingDataFromServer() {
	return m_AwaitingResponseCount > 0;
}

bool Client::IsFetchingDataFromPeer() {
	return m_AwaitingPeerResponseCount > 0;
}

uint32_t Client::GetAppVersion() {
	return m_ServerAppVersion;
}

bool Client::IsUpToDate() {
	if (m_ServerAppVersion == 0) return true;
	return g_LocalAppVersion >= m_ServerAppVersion;
}

std::string Client::GetUpdateUrl() {
	return m_UpdateUrl;
}

void Client::CMD_CHANGE_NICKNAME(SCMD::CMD_CHANGE_NICKNAME* Command) {
	if (!IsConnected())
		return;

	ENetPacket* packet = enet_packet_create(Command, sizeof(*Command), ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(m_ServerPeer, CHANNEL_REQUEST_RESPONSE, packet);
	LOGI("Sending request for command CMD_CHANGE_NICKNAME");
	m_AwaitingResponseCount++;
}

void Client::CMD_OPEN_ROOM(std::string Name) {
	if (!IsConnected())
		return;

	SCMD::CMD_OPEN_ROOM command = {};
	std::snprintf(command.Name, sizeof(command.Name), "%s", Name.c_str());

	ENetPacket* packet = enet_packet_create(&command, sizeof(command), ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(m_ServerPeer, CHANNEL_REQUEST_RESPONSE, packet);
	LOGI("Sending request for command CMD_OPEN_ROOM");
	m_AwaitingResponseCount++;
	m_RoomName = Name;
}

void Client::CMD_JOIN_ROOM(std::string Name) {
	if (!IsConnected())
		return;

	SCMD::CMD_JOIN_ROOM command = {};
	std::snprintf(command.Name, sizeof(command.Name), "%s", Name.c_str());

	ENetPacket* packet = enet_packet_create(&command, sizeof(command), ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(m_ServerPeer, CHANNEL_REQUEST_RESPONSE, packet);
	LOGI("Sending request for command CMD_JOIN_ROOM");
	m_AwaitingResponseCount++;
	m_RoomName = Name;
}

void Client::CMD_LEAVE_ROOM() {
	if (!IsConnected())
		return;

	SCMD::CMD_LEAVE_ROOM command = {};

	ENetPacket* packet = enet_packet_create(&command, sizeof(command), ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(m_ServerPeer, CHANNEL_REQUEST_RESPONSE, packet);
	LOGI("Sending request for command CMD_LEAVE_ROOM");
	m_AwaitingResponseCount++;
	m_RoomName = "";

	auto peers = m_PeerRegistry.GetAllPeers();
	for (auto& peer : peers) {
		if (!peer) continue;
		auto controllerData = &(*m_PeerControllers)[m_PeerRegistry.GetPeerId(peer)];
		if (controllerData->AllowedToReceive) {
			controllerData->Disconnected = true;
			controllerData->Settings = {};
			controllerData->PrevSimpleSettings = {};
		}
		if (controllerData->AllowedToSend) {
			(*m_PeerControllers).erase(m_PeerRegistry.GetPeerId(peer));
		}
		m_PeerRequestStatus[m_PeerRegistry.GetPeerId(peer)] = PEER_REQUEST_STATUS::PEER_NONE;
		m_PeerRegistry.Remove(peer);
	}
}

void Client::CMD_PEER_REQUEST_VIGEM(uint32_t PeerId, CONTROLLER Controller) {
	SCMD::CMD_PEER_REQUEST_VIGEM command = {};
	command.Controller = Controller;

	auto peer = m_PeerRegistry.GetPeerPtr(PeerId);
	if (!peer) return;

	ENetPacket* packet = enet_packet_create(&command, sizeof(command), ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(peer, CHANNEL_REQUEST_RESPONSE, packet);
	m_PeerRequestStatus[PeerId] = PEER_REQUEST_STATUS::WAITING_FOR_PEER_RESPONSE;
}

void Client::CMD_SEND_LOCAL_IPANDPORT() {
	ENetAddress localAddr;
	enet_socket_get_address(m_Host->socket, &localAddr);

	SCMD::CMD_SEND_LOCAL_IPANDPORT command = {};

	command.Port = localAddr.port;
	std::snprintf(command.Ip, sizeof(command.Ip), "%s", m_LocalIp.c_str());

	LOGI("Sending request for command CMD_SEND_LOCAL_IPANDPORT");
	ENetPacket* packet = enet_packet_create(&command, sizeof(command), ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(m_ServerPeer, CHANNEL_REQUEST_RESPONSE, packet);
	m_AwaitingResponseCount++;
}

void Client::CMD_PEER_ABORT_VIGEM(uint32_t PeerId) {
	auto peer = m_PeerRegistry.GetPeerPtr(PeerId);
	if (!peer) return;
	SCMD::CMD_PEER_ABORT_VIGEM command = {};
	LOGI("Sending request for command CMD_PEER_ABORT_VIGEM");
	ENetPacket* packet = enet_packet_create(&command, sizeof(command), ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(peer, CHANNEL_REQUEST_RESPONSE, packet);
}

void Client::AcceptPeerRequest(uint32_t PeerId) {
	if (m_PeerRequestStatus[PeerId] != PEER_REQUEST_STATUS::PEER_WAITING_FOR_MY_RESPONSE) return;

	SCMD::CMD_CODE_RESPONSE response = {};
	response.Cmd = CMD::CMD_PEER_REQUEST_VIGEM;

	PeerControllerData& data = (*m_PeerControllers)[PeerId];
	data.AllowedToReceive = true;

	ENetPacket* packet = enet_packet_create(&response, sizeof(response), ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(m_PeerRegistry.GetPeerPtr(PeerId), CHANNEL_REQUEST_RESPONSE, packet);
	m_PeerRequestStatus[PeerId] = PEER_REQUEST_STATUS::PEER_TRANSMITING_TO_ME;
}

void Client::DeclinePeerRequest(uint32_t PeerId) {
	if (m_PeerRequestStatus[PeerId] != PEER_REQUEST_STATUS::PEER_WAITING_FOR_MY_RESPONSE) return;

	SCMD::CMD_CODE_RESPONSE response = {};
	response.Cmd = CMD::CMD_PEER_REQUEST_VIGEM;
	response.Code = RESPONSE_CODE::E_PEER_DECLINE;

	ENetPacket* packet = enet_packet_create(&response, sizeof(response), ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(m_PeerRegistry.GetPeerPtr(PeerId), CHANNEL_REQUEST_RESPONSE, packet);
	m_PeerRequestStatus[PeerId] = PEER_REQUEST_STATUS::PEER_NONE;
}

std::vector<SCMD::CMD_CODE_RESPONSE> Client::GetResponseQueue() {
	std::lock_guard<std::mutex> guard(m_ResponseQueueMutex);
	return m_ResponseQueue;
}

void Client::PopBackResponseQueue() {
	std::lock_guard<std::mutex> guard(m_ResponseQueueMutex);
	m_ResponseQueue.pop_back();
}

SCMD::CMD_CODE_RESPONSE Client::GetLastResponseInQueue() {
	std::lock_guard<std::mutex> guard(m_ResponseQueueMutex);
	if (m_ResponseQueue.empty())
		return SCMD::CMD_CODE_RESPONSE{};

	return m_ResponseQueue.back();
}

bool Client::IsResponseQueueEmpty() {
	std::lock_guard<std::mutex> guard(m_ResponseQueueMutex);
	return m_ResponseQueue.empty();
}

bool Client::IsInRoom() {
	return m_IsInRoom;
}

std::string Client::GetRoomName() {
	return m_RoomName;
}

uint32_t Client::GetPingFromPeer(uint32_t Id) {
	auto peer = m_PeerRegistry.GetPeerPtr(Id);
	if (peer == nullptr) return 999;
	return peer->roundTripTime;
}

bool Client::IsConnectionOccupied() {
	return m_ConnectionOccupied;
}

void Client::SetSelectedController(uint32_t SelectedController) {
	m_SelectedController = SelectedController;
}

std::string Client::GetActiveLocalIP() {
#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

	std::string localIP = "127.0.0.1";

	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock >= 0) {
		sockaddr_in remoteAddr{};
		remoteAddr.sin_family = AF_INET;
		remoteAddr.sin_port = htons(53); // DNS port (doesn't matter)
		inet_pton(AF_INET, "8.8.8.8", &remoteAddr.sin_addr);

		if (connect(sock, (sockaddr*)&remoteAddr, sizeof(remoteAddr)) == 0) {
			sockaddr_in localAddr{};
			socklen_t addrLen = sizeof(localAddr);
			if (getsockname(sock, (sockaddr*)&localAddr, &addrLen) == 0) {
				char ipStr[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &localAddr.sin_addr, ipStr, sizeof(ipStr));
				localIP = ipStr;
			}
		}

	#ifdef _WIN32
		closesocket(sock);
		WSACleanup();
	#else
		close(sock);
	#endif
	}

	return localIP;
}

std::string Client::GetExternalIP() {
	if (!m_Host) return "";
	char CexternalIp[40];
	enet_address_get_host_ip(&m_Host->address, CexternalIp, 40);
	std::string externalIp = std::string(CexternalIp);
	return externalIp;
}

PEER_REQUEST_STATUS Client::GetRequestStatus(uint32_t PeerId) {
	auto it = m_PeerRequestStatus.find(PeerId);
	if (it != m_PeerRequestStatus.end()) {
		return it->second;
	}
	return PEER_REQUEST_STATUS::PEER_NONE;
}

uint32_t Client::GetGlobalPeerCount() {
	return m_GlobalPeerCount;
}

std::vector<uint32_t> Client::GetConnectedPeers() {
	return m_PeerRegistry.GetAllPeerIds();
}

std::vector<std::pair<uint32_t, std::string>> Client::GetPeerList() {
	std::vector<std::pair<uint32_t, std::string>> list;

	for (auto& peerId : m_PeerRegistry.GetAllPeerIds()) {
		list.push_back(std::pair<uint32_t, std::string>(peerId, m_PeerRegistry.GetPeerName(peerId)));
	}

	return list;
}

std::shared_ptr<std::unordered_map<uint32_t, PeerControllerData>> Client::GetActivePeerControllerMap() {
	return m_PeerControllers;
}

void Client::HostService() {
	bool sentLocalIp = false;

	while (m_ThreadRunning) {
		if (!m_Host) {
			std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;
		}

		ENetEvent evt;

		if (enet_host_service(m_Host, &evt, 1) > 0) {
			std::string ip = GetPeerFullAddress(evt.peer);
			uint32_t peerId = m_PeerRegistry.GetPeerByStrAddress(ip);

			if (evt.type == ENET_EVENT_TYPE_CONNECT) {
				if (evt.peer == m_ServerPeer) {

					CMD_GET_APP_VERSION();

					if (!sentLocalIp) {
						CMD_SEND_LOCAL_IPANDPORT();
						sentLocalIp = true;
					}

					LOGI("Connected to server [%s]", ip.c_str());
					m_Connected = true;
					m_Connecting = false;
				}
				else {
					LOGI("Connected to peer [%s]", ip.c_str());
				}
			}
			else if (evt.type == ENET_EVENT_TYPE_DISCONNECT) {
				if (evt.peer == m_ServerPeer) {
					LOGI("Disconnected from server %s", ip.c_str());
					m_Connected = false;
					m_Connecting = false;
					m_AwaitingResponseCount = 0;
					sentLocalIp = false;
				}
				else {
					(*m_PeerControllers)[peerId].Disconnected = true;
					m_PeerRegistry.Remove(peerId);
					LOGI("Disconnected from peer %s", ip.c_str());
				}
			}
			else if (evt.type == ENET_EVENT_TYPE_DISCONNECT_TIMEOUT) {
				if (evt.peer == m_ServerPeer) {
					LOGI("Disconnected from server %s (timeout)", ip.c_str());
					m_Connected = false;
					m_Connecting = false;
					m_AwaitingResponseCount = 0;
				}
				else {
					(*m_PeerControllers)[peerId].Disconnected = true;
					m_PeerRegistry.Remove(peerId);
					LOGI("Disconnected from peer %s", ip.c_str());
				}
			}
			else if (evt.type == ENET_EVENT_TYPE_RECEIVE) {
				if (evt.packet->dataLength < 1)
					continue;

				CMD packetType = (CMD)evt.packet->data[0];

				if (evt.peer == m_ServerPeer) {

					switch (packetType) {
						case CMD::CMD_RESPONSE:
						{
							SCMD::CMD_CODE_RESPONSE response = {};
							std::memcpy(&response, evt.packet->data, sizeof(response));
							LOGI("Received response for command %s", CMDToString(response.Cmd).c_str());

							{
								std::lock_guard<std::mutex> guard(m_ResponseQueueMutex);
								m_ResponseQueue.push_back(response);
							}

							if ((response.Cmd == CMD::CMD_JOIN_ROOM || response.Cmd == CMD::CMD_OPEN_ROOM) && response.Code == RESPONSE_CODE::E_SUCCESS) {
								m_IsInRoom = true;
							}
							else if ((response.Cmd == CMD::CMD_LEAVE_ROOM) && response.Code == RESPONSE_CODE::E_SUCCESS) {
								m_IsInRoom = false;
							}

							if (m_AwaitingResponseCount != 0) m_AwaitingResponseCount--;
							break;
						}

						case CMD::CMD_ACTIVE_JOIN_ROOM:
						{
							SCMD::CMD_ACTIVE_JOIN_ROOM command = {};
							std::memcpy(&command, evt.packet->data, sizeof(command));
							LOGI("Received command CMD_ACTIVE_JOIN_ROOM");
							CMD_ACTIVE_JOIN_ROOM(&command);
							break;
						}

						case CMD::CMD_ACTIVE_LEAVE_ROOM:
						{
							SCMD::CMD_ACTIVE_LEAVE_ROOM command = {};
							std::memcpy(&command, evt.packet->data, sizeof(command));
							LOGI("Received command CMD_ACTIVE_LEAVE_ROOM");
							CMD_ACTIVE_LEAVE_ROOM(&command);
							break;
						}

						case CMD::CMD_GET_PEER_COUNT:
						{
							SCMD::CMD_GET_PEER_COUNT command = {};
							std::memcpy(&command, evt.packet->data, sizeof(command));
							m_GlobalPeerCount = command.Count;
							LOGI("Received response for command CMD_GET_PEER_COUNT");
							break;
						}

						case CMD::CMD_GET_APP_VERSION:
						{
							SCMD::CMD_GET_APP_VERSION command = {};
							std::memcpy(&command, evt.packet->data, sizeof(command));
							m_ServerAppVersion = command.Version;
							m_UpdateUrl = std::string(command.UpdateUrl, strnlen(command.UpdateUrl, MAX_URL_SIZE));
							LOGI("Received response for command CMD_GET_APP_VERSION");

							if (!IsUpToDate()) {
								LOGI("Application is out of date, disconnecting from server");
								enet_peer_disconnect(m_ServerPeer, 0);
							}
							break;
						}
					}
				}
				else {
					//LOGI("Received from peer %d - %s", peerId, CMDToString(packetType).c_str());
					// Peer
					if (packetType == CMD::CMD_PING) {
						LOGI("CMD_PING received");
					}

					switch (packetType) {
						case CMD::CMD_RESPONSE:
						{
							SCMD::CMD_CODE_RESPONSE response = {};
							std::memcpy(&response, evt.packet->data, sizeof(response));
							LOGI("Received response for command %s", CMDToString(response.Cmd).c_str());

							{
								std::lock_guard<std::mutex> guard(m_ResponseQueueMutex);
								m_ResponseQueue.push_back(response);
							}

							if (response.Cmd == CMD::CMD_PEER_REQUEST_VIGEM && response.Code == RESPONSE_CODE::E_SUCCESS) {
								if (m_PeerControllers) {
									PeerControllerData& data = (*m_PeerControllers)[peerId];
									data.AllowedToSend = true;
								}
								m_PeerRequestStatus[peerId] = PEER_REQUEST_STATUS::ME_TRANSMITTING_TO_PEER;
							}
							else if (response.Cmd == CMD::CMD_PEER_REQUEST_VIGEM && response.Code != RESPONSE_CODE::E_SUCCESS) {
								m_PeerRequestStatus[peerId] = PEER_REQUEST_STATUS::PEER_DECLINED;
							}

							if (response.Cmd == CMD::CMD_PEER_ABORT_VIGEM && response.Code == RESPONSE_CODE::E_SUCCESS) {
								RemovePeerControllerData(peerId);
							}

							if (m_AwaitingPeerResponseCount != 0) m_AwaitingPeerResponseCount--;
							break;
						}
						case CMD::CMD_PEER_REQUEST_VIGEM:
						{
							SCMD::CMD_PEER_REQUEST_VIGEM command = {};
							std::memcpy(&command, evt.packet->data, sizeof(command));
							LOGI("Received command %s", CMDToString(command.Cmd).c_str());

							if (!AllowedToHostController) {
								SCMD::CMD_CODE_RESPONSE response = {};
								response.Cmd = CMD::CMD_PEER_REQUEST_VIGEM;
								response.Code = RESPONSE_CODE::E_PEER_CANT_EMULATE;
								ENetPacket* packet = enet_packet_create(&response, sizeof(response), ENET_PACKET_FLAG_RELIABLE);
								enet_peer_send(evt.peer, CHANNEL_REQUEST_RESPONSE, packet);
							}

							PeerControllerData& data = (*m_PeerControllers)[peerId];
							data.Controller = command.Controller;

							m_PeerRequestStatus[peerId] = PEER_REQUEST_STATUS::PEER_WAITING_FOR_MY_RESPONSE;
							break;
						}
						case CMD::CMD_PEER_INPUT_STATE:
						{
							if ((*m_PeerControllers)[peerId].AllowedToReceive) {
								SCMD::CMD_PEER_INPUT_STATE command = {};
								std::memcpy(&command, evt.packet->data, sizeof(command));
								std::memcpy(&(*m_PeerControllers)[peerId].InputState, &command.InputData, sizeof(s_ScePadData));
							}
							break;
						}
						case CMD::CMD_PEER_GIMMICK_STATE:
						{
							if ((*m_PeerControllers)[peerId].AllowedToSend) {
								SCMD::CMD_PEER_GIMMICK_STATE command = {};
								std::memcpy(&command, evt.packet->data, sizeof(command));
								std::memcpy(&(*m_PeerControllers)[peerId].Vibration, &command.VibrationParam, sizeof(s_ScePadVibrationParam));
								std::memcpy(&(*m_PeerControllers)[peerId].Lightbar, &command.Lightbar, sizeof(s_SceLightBar));
								m_ScePadSettings[m_SelectedController].rumbleFromEmulatedController = command.VibrationParam;
								m_ScePadSettings[m_SelectedController].lightbarFromEmulatedController = command.Lightbar;
								//LOGI("Vibration received from peer: %d, %d", m_ScePadSettings[m_SelectedController].rumbleFromEmulatedController.largeMotor, m_ScePadSettings[m_SelectedController].rumbleFromEmulatedController.smallMotor);
							}
							break;
						}
						case CMD::CMD_PEER_SETTINGS_STATE:
						{
							if ((*m_PeerControllers)[peerId].AllowedToReceive) {
								SCMD::CMD_PEER_SETTINGS_STATE command = {};
								std::memcpy(&command, evt.packet->data, sizeof(command));
								auto& simpleSettings = (*m_PeerControllers)[peerId].SimpleSettings;
								auto& settings = (*m_PeerControllers)[peerId].Settings;
								simpleSettings = command.Settings;
								settings.leftStickDeadzone = simpleSettings.leftStickDeadzone;
								settings.rightStickDeadzone = simpleSettings.rightStickDeadzone;
								settings.leftTriggerThreshold = simpleSettings.leftTriggerThreshold;
								settings.rightTriggerThreshold = simpleSettings.rightTriggerThreshold;
								settings.gyroToRightStick = simpleSettings.gyroToRightStick;
								settings.gyroToRightStickActivationButton = simpleSettings.gyroToRightStickActivationButton;
								settings.gyroToRightStickDeadzone = simpleSettings.gyroToRightStickDeadzone;
								settings.gyroToRightStickSensitivity = simpleSettings.gyroToRightStickSensitivity;
							}
							break;
						}
						case CMD::CMD_PEER_ABORT_VIGEM:
						{
							RemovePeerControllerData(peerId);

							SCMD::CMD_CODE_RESPONSE response = {};
							response.Cmd = CMD::CMD_PEER_ABORT_VIGEM;
							response.Code = RESPONSE_CODE::E_SUCCESS;
							ENetPacket* packet = enet_packet_create(&response, sizeof(response), ENET_PACKET_FLAG_RELIABLE);
							enet_peer_send(evt.peer, CHANNEL_REQUEST_RESPONSE, packet);

							break;
						}
					}
				}

				enet_packet_destroy(evt.packet);
			}
		}

		static std::chrono::steady_clock::time_point lastTimePeerCountRequest = std::chrono::steady_clock::now() - std::chrono::seconds(30);
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		if (m_Connected && (now - lastTimePeerCountRequest) > std::chrono::seconds(30)) {
			CMD_GET_PEER_COUNT();
			lastTimePeerCountRequest = now;
		}
	}
}

void Client::InputStateSendoutService() {
#ifdef WINDOWS
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	timeBeginPeriod(1);

	EXECUTION_STATE prevState = SetThreadExecutionState(
		ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED
	);

	HANDLE hTimer = CreateWaitableTimerEx(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
	LARGE_INTEGER liDueTime;
#endif
	static std::chrono::steady_clock::time_point lastTimeSent = std::chrono::steady_clock::now() - std::chrono::seconds(10); // last time input sent to a peer
	while (m_ThreadRunning) {
		if (!m_Host) {
			std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;
		}

		if (m_SelectedController < 0 || m_SelectedController > 3) return;
		s_ScePadData InputState = { };
		InputState.LeftStick.X = 128; InputState.LeftStick.Y = 128;
		InputState.RightStick.X = 128; InputState.RightStick.Y = 128;
		int result = scePadReadState(g_scePad[m_SelectedController], &InputState);

		for (auto& it : *m_PeerControllers) {
			auto now = std::chrono::steady_clock::now();
			if (it.second.AllowedToSend) {
				CMD_PEER_INPUT_STATE(it.first, InputState);

				auto& simpleSettings = it.second.SimpleSettings;
				auto& settings = m_ScePadSettings[m_SelectedController];
				simpleSettings.leftStickDeadzone = settings.leftStickDeadzone;
				simpleSettings.rightStickDeadzone = settings.rightStickDeadzone;
				simpleSettings.leftTriggerThreshold = settings.leftStickDeadzone;
				simpleSettings.rightTriggerThreshold = settings.rightStickDeadzone;
				simpleSettings.gyroToRightStick = settings.gyroToRightStick;
				simpleSettings.gyroToRightStickActivationButton = settings.gyroToRightStickActivationButton;
				simpleSettings.gyroToRightStickDeadzone = settings.gyroToRightStickDeadzone;
				simpleSettings.gyroToRightStickSensitivity = settings.gyroToRightStickSensitivity;
				if ((now - it.second.LastTimeSettingsSent) > std::chrono::seconds(1) && std::memcmp(&it.second.SimpleSettings, &it.second.PrevSimpleSettings, sizeof(s_ScePadSettingsSimple)) != 0) {

					CMD_PEER_SETTINGS_STATE(it.first, it.second.SimpleSettings);
					it.second.PrevSimpleSettings = it.second.SimpleSettings;
					it.second.LastTimeSettingsSent = now;
				}

				lastTimeSent = now;
			}

			if (it.second.AllowedToReceive) {
				if ((now - it.second.LastTimeGimmickSent) > std::chrono::milliseconds(20) &&
					((std::memcmp(&it.second.Vibration, &it.second.PrevVibration, sizeof(s_ScePadVibrationParam)) != 0) ||
					(std::memcmp(&it.second.Lightbar, &it.second.PrevLightbar, sizeof(s_SceLightBar)) != 0))) {

					CMD_PEER_GIMMICK_STATE(it.first, it.second.Vibration, it.second.Lightbar);
					it.second.PrevVibration = it.second.Vibration;
					it.second.PrevLightbar = it.second.Lightbar;
					it.second.LastTimeGimmickSent = now;
				}
			}
		}

		auto now = std::chrono::steady_clock::now();
		m_ScePadSettings[m_SelectedController].usingPeerController = ((now - lastTimeSent) < std::chrono::seconds(5)) ? true : false;
	#ifdef WINDOWS
		liDueTime.QuadPart = -80000LL;
		SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0);
		WaitForSingleObject(hTimer, INFINITE);
	#else
		std::this_thread::sleep_for(std::chrono::miliseconds(10));
	#endif
	}
}

void Client::CMD_ACTIVE_JOIN_ROOM(SCMD::CMD_ACTIVE_JOIN_ROOM* Command) {
	ENetPeer* peerRemote = enet_host_connect(m_Host, &Command->Address, CHANNEL_COUNT, 0);
	std::string peerName = std::string(Command->Name, strnlen(Command->Name, (Command->Name, MAX_NICKNAME_SIZE)));

	char newPeerIp[MAX_IP_ADDRESS_STRING_SIZE];
	enet_address_get_host(&Command->Address, newPeerIp, MAX_IP_ADDRESS_STRING_SIZE);

	m_PeerRegistry.Add(Command->peerId, peerName, peerRemote);
	m_PeerRequestStatus[Command->peerId] = PEER_REQUEST_STATUS::PEER_NONE;
	LOGI("Peer [%d] %s:%d attempting to join", Command->peerId, newPeerIp, Command->Address.port);
}

void Client::CMD_ACTIVE_LEAVE_ROOM(SCMD::CMD_ACTIVE_LEAVE_ROOM* Command) {
	ENetPeer* peer = m_PeerRegistry.GetPeerPtr(Command->peerId);

	if (!peer) return;
	auto controllerData = &(*m_PeerControllers)[Command->peerId];
	if (controllerData->AllowedToReceive) {
		controllerData->Disconnected = true;
		controllerData->Settings = {};
		controllerData->PrevSimpleSettings = {};
	}
	if (controllerData->AllowedToSend) {
		(*m_PeerControllers).erase(m_PeerRegistry.GetPeerId(peer));
	}
	m_PeerRequestStatus[Command->peerId] = PEER_REQUEST_STATUS::PEER_NONE;
	enet_peer_disconnect(peer, 0);
	m_PeerRegistry.Remove(peer);
}

void Client::CMD_PEER_INPUT_STATE(uint32_t PeerId, s_ScePadData InputState) {
	ENetPeer* peer = m_PeerRegistry.GetPeerPtr(PeerId);
	if (!peer) return;

	SCMD::CMD_PEER_INPUT_STATE command = {};
	command.InputData = InputState;
	ENetPacket* packet = enet_packet_create(&command, sizeof(command), 0);
	enet_peer_send(peer, CHANNEL_INPUT, packet);
}

void Client::CMD_PEER_GIMMICK_STATE(uint32_t PeerId, s_ScePadVibrationParam VibrationParam, s_SceLightBar Lightbar) {
	ENetPeer* peer = m_PeerRegistry.GetPeerPtr(PeerId);
	if (!peer) return;

	SCMD::CMD_PEER_GIMMICK_STATE command = {};
	command.VibrationParam = VibrationParam;
	command.Lightbar = Lightbar;
	ENetPacket* packet = enet_packet_create(&command, sizeof(command), 0);
	enet_peer_send(peer, CHANNEL_GIMMICK, packet);
}

void Client::CMD_PEER_SETTINGS_STATE(uint32_t PeerId, s_ScePadSettingsSimple Settings) {
	ENetPeer* peer = m_PeerRegistry.GetPeerPtr(PeerId);
	if (!peer) return;

	SCMD::CMD_PEER_SETTINGS_STATE command = {};
	command.Settings = Settings;
	ENetPacket* packet = enet_packet_create(&command, sizeof(command), ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(peer, CHANNEL_SETTINGS, packet);
}

void Client::CMD_GET_PEER_COUNT() {
	if (!m_ServerPeer) return;

	SCMD::CMD_UNK command = {};
	command.Cmd = CMD::CMD_GET_PEER_COUNT;

	LOGI("Sending request for command CMD_GET_PEER_COUNT");
	ENetPacket* packet = enet_packet_create(&command, sizeof(command), ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(m_ServerPeer, CHANNEL_REQUEST_RESPONSE, packet);
}

void Client::CMD_GET_APP_VERSION() {
	if (!m_ServerPeer) return;

	SCMD::CMD_UNK command = {};
	command.Cmd = CMD::CMD_GET_APP_VERSION;

	LOGI("Sending request for command CMD_GET_APP_VERSION");
	ENetPacket* packet = enet_packet_create(&command, sizeof(command), ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(m_ServerPeer, CHANNEL_REQUEST_RESPONSE, packet);
}

void Client::RemovePeerControllerData(uint32_t PeerId) {
	auto controllerData = &(*m_PeerControllers)[PeerId];
	if (controllerData->AllowedToReceive) {
		controllerData->Disconnected = true;
		controllerData->Settings = {};
		controllerData->PrevSimpleSettings = {};
	}
	if (controllerData->AllowedToSend) {
		(*m_PeerControllers).erase(PeerId);
	}
	m_PeerRequestStatus[PeerId] = PEER_REQUEST_STATUS::PEER_NONE;
}

void PeerRegistry::Add(uint32_t Id, const std::string& Name, ENetPeer* Peer) {
	m_PeerById[Id] = { Name, Peer };
	m_PeerByPtr[Peer] = { Name, Id };
	m_PeerIdByStrAddress[GetPeerFullAddress(Peer)] = Id;
}

void PeerRegistry::Remove(uint32_t Id) {
	auto it = m_PeerById.find(Id);
	if (it != m_PeerById.end()) {
		enet_peer_disconnect(m_PeerById[Id].second, 0);
		m_PeerIdByStrAddress.erase(GetPeerFullAddress(GetPeerPtr(Id)));
		m_PeerByPtr.erase(it->second.second);
		m_PeerById.erase(it);
	}
}

void PeerRegistry::Remove(ENetPeer* Peer) {
	auto it = m_PeerByPtr.find(Peer);
	if (it != m_PeerByPtr.end()) {
		enet_peer_disconnect(Peer, 0);
		m_PeerIdByStrAddress.erase(GetPeerFullAddress(Peer));
		m_PeerById.erase(it->second.second);
		m_PeerByPtr.erase(it);
	}
}

ENetPeer* PeerRegistry::GetPeerPtr(uint32_t Id) {
	auto it = m_PeerById.find(Id);
	if (it != m_PeerById.end()) {
		return it->second.second;
	}
	return nullptr;
}

std::vector<ENetPeer*> PeerRegistry::GetAllPeers() {
	std::vector<ENetPeer*> peers;
	peers.reserve(m_PeerById.size());
	for (const auto& [id, pair] : m_PeerById) {
		if (pair.second)
			peers.push_back(pair.second);
	}
	return peers;
}

std::vector<uint32_t> PeerRegistry::GetAllPeerIds() {
	std::vector<uint32_t> ids;
	ids.reserve(m_PeerById.size());
	for (const auto& [id, pair] : m_PeerById) {
		if (pair.second)
			ids.push_back(id);
	}
	return ids;
}

std::string PeerRegistry::GetPeerName(uint32_t Id) {
	auto it = m_PeerById.find(Id);
	if (it != m_PeerById.end()) {
		return it->second.first;
	}
	return "";
}

uint32_t PeerRegistry::GetPeerId(ENetPeer* Peer) {
	auto it = m_PeerByPtr.find(Peer);
	if (it != m_PeerByPtr.end()) {
		return it->second.second;
	}
	return 0;
}

uint32_t PeerRegistry::GetPeerByStrAddress(const std::string& Address) {
	auto it = m_PeerIdByStrAddress.find(Address);
	if (it != m_PeerIdByStrAddress.end()) {
		return it->second;
	}
	return 0;
}

std::string GetPeerFullAddress(ENetPeer* Peer) {
	char ip[MAX_IP_ADDRESS_STRING_SIZE];
	enet_address_get_host_ip(&Peer->address, ip, MAX_IP_ADDRESS_STRING_SIZE);
	std::string ipString = std::string(ip, strnlen(ip, MAX_IP_ADDRESS_STRING_SIZE));
	ipString += ":" + std::to_string(Peer->address.port);
	return ipString;
}
