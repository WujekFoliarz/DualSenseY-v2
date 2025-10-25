#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <nlohmann/json.hpp>

struct AppSettings {
	bool DisableAllBluetoothControllersOnExit = false;
	bool DontConnectToServerOnStart = false;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
	AppSettings,
	DisableAllBluetoothControllersOnExit,
	DontConnectToServerOnStart
);

void saveAppSettings(AppSettings* appSettings);
void loadAppSettings(AppSettings* appSettings);

#endif