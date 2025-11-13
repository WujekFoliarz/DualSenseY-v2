#ifndef MAINWINDOW_H  
#define MAINWINDOW_H  

#include "scePadSettings.hpp"  
#include "strings.hpp"  
#include "audioPassthrough.hpp"  
#include "udp.hpp"
#include "controllerEmulation.hpp"
#include "utils.hpp"
#include "appSettings.hpp"
#include "client.hpp"

static inline std::tm StringToTimeZone(const std::string& dateStr, int utcOffsetHours) {
	std::tm tm = {};
	std::istringstream ss(dateStr);
	ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
	if (ss.fail()) throw std::runtime_error("Failed to parse date string");

	std::time_t t = std::mktime(&tm); // interprets tm as local
	if (t == -1) throw std::runtime_error("Failed to convert to time_t");

	t += utcOffsetHours * 3600; // add offset in seconds

	std::tm target_tm;
#if defined(_WIN32)
	localtime_s(&target_tm, &t);
#else
	localtime_r(&t, &target_tm);
#endif

	return target_tm;
}

class MainWindow {
	Strings& m_Strings;
	AudioPassthrough& m_Audio;
	Vigem& m_Vigem;
	UDP& m_Udp;
	AppSettings& m_AppSettings;
	Client& m_Client;
	bool m_IsAdminWindows = IsRunningAsAdministratorWindows();
	bool& m_IsLightMode;
private:
	int m_SelectedController = 0;
	bool About(bool* open);
	bool MenuBar(int& currentController, s_scePadSettings& scePadSettings);
	bool Controllers(int& currentController, s_scePadSettings& scePadSettings, float scale);
	bool Led(s_scePadSettings& scePadSettings, float scale);
	bool Audio(int currentController, s_scePadSettings& scePadSettings);
	bool Emulation(int currentController, s_scePadSettings& scePadSettings, s_ScePadData& state);
	bool AdaptiveTriggers(s_scePadSettings& scePadSettings);
	bool KeyboardAndMouseMapping(s_scePadSettings& scePadSettings, s_ScePadData& state);
	bool Touchpad(int currentController, s_scePadSettings& scePadSettings, s_ScePadData& state, float scale);
	bool TreeElement_touchpadDiagnostics(int currentController, s_scePadSettings& scePadSettings, s_ScePadData& state, float scale);
	bool TreeElement_analogSticks(s_scePadSettings& scePadSettings, s_ScePadData& state);
	bool TreeElement_lightbar(s_scePadSettings& scePadSettings);
	bool TreeElement_vibration(s_scePadSettings& scePadSettings);
	bool TreeElement_dynamicAdaptiveTriggers(s_scePadSettings& scePadSettings);
	bool TreeElement_motion(s_scePadSettings& scePadSettings, s_ScePadData& state);
	bool TreeElement_touchpad(s_scePadSettings& scePadSettings);
	bool TreeElement_sharebtn(s_scePadSettings& scePadSettings);
	bool Online(s_scePadSettings& scePadSettings);
	bool MessageFromServer(bool* open, SCMD::CMD_CODE_RESPONSE* Response);
	bool ScreenBlock(bool open, const char* message, const char* popup_id);
	bool ScreenBlockClosable(bool* open, const char* message, const char* popup_id);
	void Errors();
	bool GetHotkeyFromControllerScreen(bool* open, int countdown, int expectedCountdownLength);
public:
	MainWindow(Strings& strings, AudioPassthrough& audio, Vigem& vigem, UDP& udp, AppSettings& appSettings, Client& client, bool& IsLightMode)
		: m_Strings(strings), m_Audio(audio), m_Vigem(vigem), m_Udp(udp), m_AppSettings(appSettings), m_Client(client), m_IsLightMode(IsLightMode) {
	}
	void Show(s_scePadSettings scePadSettings[4], float scale);
	int GetSelectedController();
};

#endif // MAINWINDOW_H