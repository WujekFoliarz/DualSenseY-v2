#include "mainWindow.hpp"

#include <imgui.h>
#include <string>

#include <cmath>
#include "log.hpp"
#include <duaLib.h>

#define str(string) m_strings.getString(string).c_str()
#define strr(string) m_strings.getString(string)

bool MainWindow::about(bool* open) {
	if (!ImGui::Begin("About DualSenseY", open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking)) {
		ImGui::End();
		return false;
	}

	ImGui::Text("Version 46");
	ImGui::Text("Made by Wujek_Foliarz");
	ImGui::Text("DualSenseY is licensed under the MIT License, see LICENSE for more information.");

	ImGui::End();
	return true;
}

bool MainWindow::menuBar() {
	static bool openAbout = false;

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu(str("File"))) {
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(str("Help"))) {
			ImGui::TextLinkOpenURL("Discord", "https://discord.gg/AFYvxf282U");
			ImGui::TextLinkOpenURL("GitHub", "https://github.com/WujekFoliarz/DualSenseY-v2/issues");
			ImGui::MenuItem(str("About"), "", &openAbout);		

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	if(openAbout) about(&openAbout);

	return true;
}

bool MainWindow::controllers(int& currentController, s_scePadSettings scePadSettings[4], float scale) {
	ImGui::SeparatorText("Controller");

	bool noneConnected = true;
	for (uint32_t i = 0; i < 4; i++) {
		s_ScePadData data = {};
		uint32_t result = scePadReadState(scePadSettings[i].handle, &data);
		if (result == SCE_OK) {
			noneConnected = false;
			ImGui::RadioButton(std::to_string(i + 1).c_str(), &currentController, i);
			ImGui::SameLine();
		}
	}
	
	if (noneConnected) {
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "No controllers connected");
		return false;
	}

	ImGui::NewLine();
	return true;
}

bool MainWindow::led(int& currentController, s_scePadSettings scePadSettings[4], float scale) {
	ImGui::SeparatorText(str("LedSection"));

	ImGui::Checkbox(str("DisablePlayerLED"), &scePadSettings[currentController].disablePlayerLed);
	ImGui::Checkbox(str("AudioToLED"), &scePadSettings[currentController].audioToLed);

	ImGui::Text(str("PlayerLedBrightness")); ImGui::SameLine();
	ImGui::RadioButton(str("High"), &scePadSettings[currentController].brightness, 0); ImGui::SameLine();
	ImGui::RadioButton(str("Medium"), &scePadSettings[currentController].brightness, 1); ImGui::SameLine();
	ImGui::RadioButton(str("Low"), &scePadSettings[currentController].brightness, 2);

	ImGui::NewLine();
	ImGui::SetNextItemWidth(scale);
	ImGui::ColorPicker3(str("LightbarColor"), scePadSettings[currentController].led, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
	return true;
}

bool MainWindow::udp(int& currentController, float scale) {
	ImGui::SeparatorText(str("DSX Mods/UDP"));
	ImGui::Text(std::string(strr("Status") + ":").c_str()); ImGui::SameLine();
	ImGui::TextColored(ImVec4(1, 0, 0, 1), m_strings.getString("Inactive").c_str());

	return true;
}

bool MainWindow::audio(int& currentController, s_scePadSettings scePadSettings[4]) {
	static bool failedToStart = false;
	int busType = 0;
	scePadGetControllerBusType(scePadSettings[currentController].handle, &busType);

	ImGui::SeparatorText(str("Audio"));

	if (busType == SCE_PAD_BUSTYPE_BT) {
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "Bluetooth");
		return true;
	}

	if (ImGui::Checkbox(str("Audio passthrough"), &scePadSettings[currentController].audioPassthrough)) {
		if (scePadSettings[currentController].audioPassthrough) {
			if (!m_audio.startByUserId(currentController + 1)) {
				LOGE("Failed to start audio passthrough");
				scePadSettings[currentController].audioPassthrough = false;
				failedToStart = true;
			}
			else {
				failedToStart = false;
			}
		}
		else {
			if (!m_audio.stopByUserId(currentController + 1)) {
				LOGE("Failed to stop audio passthrough");
			}
		}
	}

	if (failedToStart) {
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1, 0, 0, 1), str("Failed to start"));
	}
	else if (!failedToStart && scePadSettings[currentController].audioPassthrough) {
		ImGui::SetNextItemWidth(400);
		ImGui::SliderFloat(str("Haptics intensity"), &scePadSettings[currentController].hapticIntensity, 0.0f, 5.0f);
	}

	ImGui::Text(str("Audio output path"));
	ImGui::RadioButton(str("Stereo headset"), &scePadSettings[currentController].audioPath, SCE_PAD_AUDIO_PATH_STEREO_HEADSET); 
	ImGui::RadioButton(str("Mono left headset"), &scePadSettings[currentController].audioPath, SCE_PAD_AUDIO_PATH_MONO_LEFT_HEADSET);
	ImGui::RadioButton(str("Mono left headset and speaker"), &scePadSettings[currentController].audioPath, SCE_PAD_AUDIO_PATH_MONO_LEFT_HEADSET_AND_SPEAKER); 
	ImGui::RadioButton(str("Only speaker"), &scePadSettings[currentController].audioPath, SCE_PAD_AUDIO_PATH_ONLY_SPEAKER);
		
	ImGui::SetNextItemWidth(400);
	ImGui::SliderInt(str("Speaker volume"), &scePadSettings[currentController].speakerVolume, 0, 8, "%d");
	ImGui::SetNextItemWidth(400);
	ImGui::SliderInt(str("Microphone gain"), &scePadSettings[currentController].micGain, 0, 8, "%d");
	return true;
}

void MainWindow::show(s_scePadSettings scePadSettings[4], float scale) {
	static int c = 0; 
	scale = 100 * (scale * 2.5);

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);

	ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus);
	ImGui::TextColored(ImVec4(1, 0, 0, 1), "Work in progress. Older build at v2 branch on github");
	s_ScePadData state = {};
	scePadReadState(scePadSettings[c].handle, &state);

	menuBar();
	if (controllers(c, scePadSettings, scale)) {
		udp(c, scale);
		led(c, scePadSettings, scale);
		audio(c, scePadSettings);
	}
	
	ImGui::End();
}