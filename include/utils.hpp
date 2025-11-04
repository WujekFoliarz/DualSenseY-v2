#ifndef UTILS_H
#define UTILS_H

#include <string>
#include "log.hpp"
#include <filesystem>
#include <iostream>

void HidHideRequest(std::string ID, std::string arg);
std::string USBtoHIDinstance(const std::string& input);
void HideController(const std::string& instanceId);
void UnhideController(const std::string& instanceId);
bool IsRunningAsAdministratorWindows();
void DisableBluetoothDevice(const std::string& Address);

#endif