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

class MainWindow {
	Strings& m_Strings;
	AudioPassthrough& m_Audio;
	Vigem& m_Vigem;
	UDP& m_Udp;
	AppSettings& m_AppSettings;
	Client& m_Client;
	bool m_IsAdminWindows = IsRunningAsAdministratorWindows();
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
	bool Online();
	bool MessageFromServer(bool* open, SCMD::CMD_CODE_RESPONSE* Response);
	bool ScreenBlock(bool* open, const char* Message);
	void Errors();
	bool GetHotkeyFromControllerScreen(bool* open, int countdown, int expectedCountdownLength);
public:
	MainWindow(Strings& strings, AudioPassthrough& audio, Vigem& vigem, UDP& udp, AppSettings& appSettings, Client& client)
		: m_Strings(strings), m_Audio(audio), m_Vigem(vigem), m_Udp(udp), m_AppSettings(appSettings), m_Client(client) {
	}
	void Show(s_scePadSettings scePadSettings[4], float scale);
	int GetSelectedController();
};

#endif // MAINWINDOW_H