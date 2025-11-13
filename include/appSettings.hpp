#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <nlohmann/json.hpp>
#include <unordered_map>

static constexpr uint8_t FONT_REGULAR = 0;
static constexpr uint8_t FONT_JP_CYRILLIC = 1;
static constexpr uint8_t FONT_KOREAN = 2;
static constexpr uint8_t FONT_THAI = 3;
static constexpr uint8_t FONT_ARABIC = 4;
//static constexpr uint8_t FONT_CHINESE_SIMPLIFIED = 5;

static std::unordered_map<std::string, std::string> g_LanguageName = {
	{"en", "English"},
	//{"zh-CN", "简体中文"},
	//{"fr", "Français"},
	//{"de", "Deutsch"},
	//{"it", "Italiano"},
	//{"ja", "日本語"},
	//{"ko", "한국어"},
	{"pl", "Polski"},
	{"pt", "Português (Brasil)"},
	//{"ru", "Русский"},
	{"tr", "Türkçe"},
	{"es", "Español"},
	//{"tr", "Türkçe"},
	//{"uk", "Українська"},
	//{"vi", "Tiếng Việt"},
	//{"ar", "اَلْعَرَبِيَّةُ"},
};

static std::unordered_map<std::string, uint8_t> g_FontIndex = {
	{"ja", FONT_JP_CYRILLIC},
	{"ru", FONT_JP_CYRILLIC},
	{"uk", FONT_JP_CYRILLIC},
	{"ko", FONT_KOREAN},
	{"th", FONT_THAI},
	{"ar", FONT_ARABIC},
};

struct AppSettings {
	bool DisableAllBluetoothControllersOnExit = false;
	bool DontConnectToServerOnStart = false;
	std::string SelectedLanguage = "en";
	bool HideToTrayOnMinimize = false;
	bool HideToTrayOnStart = false;
	std::string ServerAddress = "maluch.mikr.us";
	uint16_t ServerPort = 30151;
	uint16_t LocalPort = 6969;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
	AppSettings,
	DisableAllBluetoothControllersOnExit,
	DontConnectToServerOnStart,
	SelectedLanguage,
	HideToTrayOnMinimize,
	HideToTrayOnStart,
	ServerAddress,
	ServerPort,
	LocalPort
);

void SaveAppSettings(AppSettings* appSettings);
void LoadAppSettings(AppSettings* appSettings);

#endif