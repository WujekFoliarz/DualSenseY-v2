#ifndef UTILS_H
#define UTILS_H

#include <string>
#include "log.hpp"
#include <filesystem>
#include <iostream>

void hidHideRequest(std::string ID, std::string arg);
std::string USBtoHIDinstance(const std::string& input);
void hideController(const std::string& instanceId);
void unhideController(const std::string& instanceId);
bool isRunningAsAdministratorWindows();
void DisableBluetoothDevice(const std::string& Address);

#endif