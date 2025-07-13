#ifndef UTILS_H
#define UTILS_H

#include <string>
#include "log.hpp"
#include <filesystem>
#include <iostream>

#if !defined(__linux__) && !defined(__MACOS__)
#include <Windows.h>


static void hidHideRequest(std::string ID, std::string arg) {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW; // Use this flag to control window visibility
	si.wShowWindow = SW_HIDE;          // Set to SW_HIDE to prevent the window from showing

	ZeroMemory(&pi, sizeof(pi));

	std::string arg1 = "\"" + ID + "\"";
	std::string arg2 = " \"" + arg + "\" ";
	
	std::filesystem::path path = std::filesystem::current_path();
	std::string exePath = "";

	if (std::filesystem::exists(path.string() + "\\DSX.exe")) {
		exePath = path.string() + "\\DSX.exe";
	}
	else {
		exePath = path.string() + "\\DualSenseY.exe";
	}

	std::string arg3 = " \"" + exePath + "\" ";

	std::string command = RESOURCES_PATH "/externals/windows/HidHide.exe " + arg1 + arg2 + arg3;

	if (CreateProcess(NULL,              // No module name (use command line)
		(LPSTR)command.c_str(), // Command line
		NULL,               // Process handle not inheritable
		NULL,               // Thread handle not inheritable
		FALSE,              // Set handle inheritance to FALSE
		0,                  // No creation flags
		NULL,               // Use parent's environment block
		NULL,               // Use parent's starting directory 
		&si,                // Pointer to STARTUPINFO structure
		&pi)                // Pointer to PROCESS_INFORMATION structure
	   ) {
		// Close process and thread handles. 
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	else {
		LOGE("Failed to start HidHide.exe");
	}
}
#endif

std::string USBtoHIDinstance(const std::string& input);
void hideController(const std::string& instanceId);
void unhideController(const std::string& instanceId);
bool isRunningAsAdministratorWindows();

#endif