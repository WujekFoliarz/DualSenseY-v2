#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <nlohmann/json.hpp>
#include <unordered_map>

static constexpr uint8_t FONT_REGULAR = 0;
static constexpr uint8_t FONT_JP_CYRILLIC = 1;
static constexpr uint8_t FONT_KOREAN = 2;
static constexpr uint8_t FONT_THAI = 3;
static constexpr uint8_t FONT_CHINESE_SIMPLIFIED = 4;

static std::unordered_map<std::string, std::string> g_LanguageName = {
	{"en", "English"},
	//{"zh-CN", "简体中文"},
	//{"fr", "Français"},
	//{"de", "Deutsch"},
	//{"it", "Italiano"},
	//{"ja", "日本語"},
	//{"ko", "한국어"},
	{"pl", "Polski"},
	//{"pt-BR", "Português (Brasil)"},
	//{"ru", "Русский"},
	{"es", "Español"},
	//{"tr", "Türkçe"},
	//{"uk", "Українська"},
	//{"vi", "Tiếng Việt"},
};

static std::unordered_map<std::string, uint8_t> g_FontIndex = {
	{"ja", 1},
	{"ru", 1},
	{"uk", 1},
	{"ko", 2},
	{"th", 3},
	{"zh-CN", 4},
};

struct AppSettings {
	bool DisableAllBluetoothControllersOnExit = false;
	bool DontConnectToServerOnStart = false;
	std::string SelectedLanguage = "en";
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
	AppSettings,
	DisableAllBluetoothControllersOnExit,
	DontConnectToServerOnStart,
	SelectedLanguage
);

void saveAppSettings(AppSettings* appSettings);
void loadAppSettings(AppSettings* appSettings);

#endif