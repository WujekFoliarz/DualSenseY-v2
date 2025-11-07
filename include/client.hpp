#pragma once
#include <enet.h>
#include <string>
#include <cstdint>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <duaLib.h>
#include <tuple>
#include <functional>
#include "scePadSettings.hpp"

constexpr auto MAX_ROOM_NAME_SIZE = 16;
constexpr auto MAX_NICKNAME_SIZE = 16;
constexpr auto MAX_IP_ADDRESS_STRING_SIZE = 40;
constexpr auto MAX_URL_SIZE = 2048;

constexpr auto CHANNEL_REQUEST_RESPONSE = 0; // Reliable
constexpr auto CHANNEL_INPUT = 1; // Unreliable
constexpr auto CHANNEL_GIMMICK = 2; // Unreliable
constexpr auto CHANNEL_SETTINGS = 3; // Reliable
constexpr auto CHANNEL_COUNT = 4;

enum class CMD : uint8_t {
	CMD_UNK,
	CMD_RESPONSE,
	CMD_GET_SERVTIME,
	CMD_CHANGE_NICKNAME,
	CMD_OPEN_ROOM,
	CMD_JOIN_ROOM,
	CMD_ACTIVE_JOIN_ROOM,
	CMD_PING,
	CMD_PONG,
	CMD_GET_PEER_INFO,
	CMD_LEAVE_ROOM,
	CMD_ACTIVE_LEAVE_ROOM,
	CMD_SEND_LOCAL_IPANDPORT,
	CMD_GET_PEER_COUNT,
	CMD_PEER_REQUEST_VIGEM,
	CMD_PEER_ABORT_VIGEM,
	CMD_PEER_INPUT_STATE,
	CMD_PEER_GIMMICK_STATE,
	CMD_PEER_SETTINGS_STATE,
	CMD_GET_APP_VERSION,
};

inline static std::string CMDToString(CMD cmd) {
	switch (cmd) {
		case CMD::CMD_UNK:						return "CMD_UNK";
		case CMD::CMD_RESPONSE:					return "CMD_RESPONSE";
		case CMD::CMD_GET_SERVTIME:				return "CMD_GET_SERVTIME";
		case CMD::CMD_CHANGE_NICKNAME:			return "CMD_CHANGE_NICKNAME";
		case CMD::CMD_OPEN_ROOM:				return "CMD_OPEN_ROOM";
		case CMD::CMD_JOIN_ROOM:				return "CMD_JOIN_ROOM";
		case CMD::CMD_ACTIVE_JOIN_ROOM:			return "CMD_ACTIVE_JOIN_ROOM";
		case CMD::CMD_PING:						return "CMD_PING";
		case CMD::CMD_PONG:						return "CMD_PONG";
		case CMD::CMD_GET_PEER_INFO:			return "CMD_GET_PEER_INFO";
		case CMD::CMD_LEAVE_ROOM:				return "CMD_LEAVE_ROOM";
		case CMD::CMD_ACTIVE_LEAVE_ROOM:		return "CMD_ACTIVE_LEAVE_ROOM";
		case CMD::CMD_PEER_REQUEST_VIGEM:		return "CMD_PEER_REQUEST_VIGEM";
		case CMD::CMD_PEER_ABORT_VIGEM:			return "CMD_PEER_ABORT_VIGEM";
		case CMD::CMD_SEND_LOCAL_IPANDPORT:		return "CMD_SEND_LOCAL_IPANDPORT";
		case CMD::CMD_GET_PEER_COUNT:			return "CMD_GET_PEER_COUNT";
		case CMD::CMD_PEER_INPUT_STATE:			return "CMD_PEER_INPUT_STATE";
		case CMD::CMD_PEER_GIMMICK_STATE:		return "CMD_PEER_GIMMICK_STATE";
		case CMD::CMD_PEER_SETTINGS_STATE:		return "CMD_PEER_SETTINGS_STATE";
		default:								return "UNKNOWN_CMD";
	}
}

enum class CONTROLLER : uint8_t {
	XBOX360,
	DUALSHOCK4
};

enum class RESPONSE_CODE : uint8_t {
	E_SUCCESS,
	E_PEER_ALREADY_IN_ROOM,
	E_PEER_UNAVAILABLE,
	E_PEER_CANT_EMULATE, // Non-windows or no Vigem installed
	E_PEER_DECLINE,

	E_ROOM_FULL,
	E_ROOM_DOESNT_EXIST,
	E_ROOM_ALREADY_EXISTS,
	E_ROOM_NAME_EMPTY,

	E_SERVER_ERROR,
};

enum class PEER_REQUEST_STATUS : uint8_t {
	PEER_NONE,
	PEER_TRANSMITING_TO_ME,
	ME_TRANSMITTING_TO_PEER,
	PEER_DECLINED,
	WAITING_FOR_PEER_RESPONSE,
	PEER_WAITING_FOR_MY_RESPONSE,
};

namespace SCMD {
#pragma pack(push, 1)
	struct CMD_UNK {
		CMD Cmd = CMD::CMD_UNK;
	};

	struct CMD_OPEN_ROOM {
		CMD Cmd = CMD::CMD_OPEN_ROOM;
		char Name[MAX_ROOM_NAME_SIZE] = { 0 };
	};

	struct CMD_JOIN_ROOM {
		CMD Cmd = CMD::CMD_JOIN_ROOM;
		char Name[MAX_ROOM_NAME_SIZE] = { 0 };
	};

	struct CMD_ACTIVE_JOIN_ROOM {
		CMD Cmd = CMD::CMD_ACTIVE_JOIN_ROOM;
		char Name[MAX_NICKNAME_SIZE] = { 0 };
		ENetAddress Address = { 0 };
		uint32_t peerId = 0;
	};

	struct CMD_CHANGE_NICKNAME {
		CMD Cmd = CMD::CMD_CHANGE_NICKNAME;
		char Name[MAX_NICKNAME_SIZE] = { 0 };
	};

	struct CMD_CODE_RESPONSE {
		CMD ResponseCmd = CMD::CMD_RESPONSE;
		CMD Cmd = CMD::CMD_UNK;
		RESPONSE_CODE Code = RESPONSE_CODE::E_SUCCESS;
	};

	struct CMD_PING {
		CMD Cmd = CMD::CMD_PING;
	};

	struct CMD_PONG {
		CMD Cmd = CMD::CMD_PONG;
	};

	struct CMD_LEAVE_ROOM {
		CMD Cmd = CMD::CMD_LEAVE_ROOM;
	};

	struct CMD_ACTIVE_LEAVE_ROOM {
		CMD Cmd = CMD::CMD_ACTIVE_LEAVE_ROOM;
		uint32_t peerId = 0;
	};

	struct CMD_PEER_REQUEST_VIGEM {
		CMD Cmd = CMD::CMD_PEER_REQUEST_VIGEM;
		CONTROLLER Controller = CONTROLLER::XBOX360;
	};

	struct CMD_PEER_INPUT_STATE {
		CMD Cmd = CMD::CMD_PEER_INPUT_STATE;
		s_ScePadData InputData = {};
	};

	struct CMD_SEND_LOCAL_IPANDPORT {
		CMD Cmd = CMD::CMD_SEND_LOCAL_IPANDPORT;
		char Ip[MAX_IP_ADDRESS_STRING_SIZE] = { 0 };
		enet_uint16 Port = 0;
	};

	struct CMD_PEER_GIMMICK_STATE {
		CMD Cmd = CMD::CMD_PEER_GIMMICK_STATE;
		s_ScePadVibrationParam VibrationParam = {};
		s_SceLightBar Lightbar = {};
	};

	struct CMD_PEER_SETTINGS_STATE {
		CMD Cmd = CMD::CMD_PEER_SETTINGS_STATE;
		s_ScePadSettingsSimple Settings = {};
	};

	struct CMD_PEER_ABORT_VIGEM {
		CMD Cmd = CMD::CMD_PEER_ABORT_VIGEM;
	};

	struct CMD_GET_PEER_COUNT {
		CMD Cmd = CMD::CMD_GET_PEER_COUNT;
		uint32_t Count = 0;
	};

	struct CMD_GET_APP_VERSION {
		CMD Cmd = CMD::CMD_GET_APP_VERSION;
		uint32_t Version = 0;
		char UpdateUrl[MAX_URL_SIZE] = { 0 };
	};
#pragma pack(pop)
}

struct PeerControllerData {
	bool AllowedToSend = false;
	bool AllowedToReceive = false;
	bool Disconnected = false;
	CONTROLLER Controller = CONTROLLER::XBOX360;
	s_ScePadData InputState = {};
	s_ScePadVibrationParam Vibration = {};
	s_SceLightBar Lightbar = {};
	s_scePadSettings Settings = {};
	s_ScePadSettingsSimple SimpleSettings = {};
	s_SceLightBar PrevLightbar = {};
	s_ScePadVibrationParam PrevVibration = {};
	s_ScePadSettingsSimple PrevSimpleSettings = {};
	std::mutex Lock;
	std::chrono::steady_clock::time_point LastTimeSettingsSent = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point LastTimeGimmickSent = std::chrono::steady_clock::now();
};

std::string GetPeerFullAddress(ENetPeer* Peer);

class PeerRegistry {
public:
	void Add(uint32_t Id, const std::string& Name, ENetPeer* Peer);
	void Remove(uint32_t Id);
	void Remove(ENetPeer* Peer);
	ENetPeer* GetPeerPtr(uint32_t Id);
	std::vector<ENetPeer*> GetAllPeers();
	std::vector<uint32_t> GetAllPeerIds();
	std::string GetPeerName(uint32_t Id);
	uint32_t GetPeerId(ENetPeer* Peer);
	uint32_t GetPeerByStrAddress(const std::string& Address);
private:
	std::unordered_map<uint32_t, std::pair<std::string, ENetPeer*>> m_PeerById;
	std::unordered_map<ENetPeer*, std::pair<std::string, uint32_t>> m_PeerByPtr;
	std::unordered_map<std::string, uint32_t> m_PeerIdByStrAddress;
};

class Client {
public:
	Client(s_scePadSettings* ScePadSettings);
	~Client();
	void Connect(const std::string& Ip, uint16_t Port);
	void Start();
	bool IsConnected();
	bool IsConnecting();
	bool IsFetchingDataFromServer();
	bool IsFetchingDataFromPeer();

	void CMD_CHANGE_NICKNAME(SCMD::CMD_CHANGE_NICKNAME* Command);
	void CMD_OPEN_ROOM(std::string Name);
	void CMD_JOIN_ROOM(std::string Name);
	void CMD_LEAVE_ROOM();
	void CMD_PEER_REQUEST_VIGEM(uint32_t PeerId, CONTROLLER Controller);
	void CMD_SEND_LOCAL_IPANDPORT();
	void CMD_PEER_ABORT_VIGEM(uint32_t PeerId);

	void AcceptPeerRequest(uint32_t PeerId);
	void DeclinePeerRequest(uint32_t PeerId);

	std::vector<SCMD::CMD_CODE_RESPONSE> GetResponseQueue();
	void PopBackResponseQueue();
	SCMD::CMD_CODE_RESPONSE GetLastResponseInQueue();
	bool IsResponseQueueEmpty();
	bool IsInRoom();
	std::string GetRoomName();
	uint32_t GetPingFromPeer(uint32_t Id);
	bool IsConnectionOccupied();
	void SetSelectedController(uint32_t SelectedController);
	std::string GetActiveLocalIP();
	std::string GetExternalIP();
	PEER_REQUEST_STATUS GetRequestStatus(uint32_t PeerId);
	uint32_t GetGlobalPeerCount(); // This returns count of people connected to the central server, not the room that you're in.
	uint32_t GetAppVersion();
	bool IsUpToDate(); // Returns true if we haven't fetched server app version yet
	std::string GetUpdateUrl();

	std::vector<uint32_t> GetConnectedPeers();
	std::vector<std::pair<uint32_t, std::string>> GetPeerList();
	std::shared_ptr<std::unordered_map<uint32_t, PeerControllerData>> GetActivePeerControllerMap();
	bool AllowedToHostController = false;
private:
	void HostService();
	void InputStateSendoutService();

	void CMD_ACTIVE_JOIN_ROOM(SCMD::CMD_ACTIVE_JOIN_ROOM* Command);
	void CMD_ACTIVE_LEAVE_ROOM(SCMD::CMD_ACTIVE_LEAVE_ROOM* Command);
	void CMD_PEER_INPUT_STATE(uint32_t PeerId, s_ScePadData InputState);
	void CMD_PEER_GIMMICK_STATE(uint32_t PeerId, s_ScePadVibrationParam VibrationParam, s_SceLightBar Lightbar);
	void CMD_PEER_SETTINGS_STATE(uint32_t PeerId, s_ScePadSettingsSimple Settings);
	void CMD_GET_PEER_COUNT(); // Central server peer count
	void CMD_GET_APP_VERSION();

	void RemovePeerControllerData(uint32_t PeerId);

	std::shared_ptr<std::unordered_map<uint32_t, PeerControllerData>> m_PeerControllers;
	std::atomic<bool> m_IsInRoom = false;
	std::string m_RoomName = "";
	std::vector<SCMD::CMD_CODE_RESPONSE> m_ResponseQueue;
	std::atomic<uint32_t> m_AwaitingResponseCount = 0;
	std::atomic<uint32_t> m_AwaitingPeerResponseCount = 0;
	std::unordered_map<uint32_t, PEER_REQUEST_STATUS> m_PeerRequestStatus;
	std::mutex m_ResponseQueueMutex;
	ENetPeer* m_ServerPeer = nullptr;
	ENetHost* m_Host = nullptr;
	std::atomic<bool> m_ThreadRunning = false;
	std::atomic<bool> m_Connected = false;
	std::atomic<bool> m_Connecting = false;
	bool m_ConnectionOccupied = false;
	uint32_t m_SelectedController = 0;
	std::string m_LocalIp = "127.0.0.1";
	s_scePadSettings* m_ScePadSettings = nullptr;
	uint32_t m_GlobalPeerCount = 0;
	uint32_t m_ServerAppVersion = 0;
	std::string m_UpdateUrl = "";

	std::thread m_ServiceThread;
	std::thread m_InputStateSendoutThread;
	PeerRegistry m_PeerRegistry;
};