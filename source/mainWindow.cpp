#include "mainWindow.hpp"

#include <imgui.h>
#include <string>

#include <cmath>
#include "log.hpp"
#include <duaLib.h>
#include "scePadHandle.hpp"
#include "utils.hpp"
#include <nfd.h>
#include <platform_folders.h>
#include <filesystem>
#include <fstream>

#define str(string) m_strings.getString(string).c_str()
#define strr(string) m_strings.getString(string)

bool MainWindow::about(bool* open) {
	ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));

	if (!ImGui::Begin("About DualSenseY", open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking)) {
		ImGui::PopStyleColor(2);
		ImGui::End();
		return false;
	}

	ImGui::Text("Version 46");
	ImGui::Text("Made by Wujek_Foliarz");
	ImGui::Text("DualSenseY is licensed under the MIT License,");
	ImGui::Text("see LICENSE for more information.");

	//ImGui::NewLine();
	//ImGui::Text("ążźćłó こにちは 안녕하세요 Привет สวัสดี äöüßéèáç");

	ImGui::End();
	ImGui::PopStyleColor(2);
	return true;
}

static bool showLoadFailedError = false;
static bool showSetDefaultConfigSuccess = false;
static bool showControllerNotConnectedError = false;
bool MainWindow::menuBar(int& currentController, s_scePadSettings& scePadSettings) {
	static bool openAbout = false;

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu(str("File"))) {

			if (ImGui::MenuItem(str("Save"))) {
				nfdchar_t* outPath = NULL;
				nfdresult_t result = NFD_SaveDialog("dsy", NULL, &outPath);

				if (result == NFD_OKAY) {
					std::string outPathString(outPath);
					if (outPathString.find(".dsy") == std::string::npos) {
						outPathString += ".dsy";
					}
					saveSettingsToFile(scePadSettings, outPathString);
					free(outPath);
				}
				else {
					LOGE("Failed to open save dialog: %d", result);
				}
			}

			if (ImGui::MenuItem(str("Load"))) {

				nfdchar_t* outPath = NULL;
				nfdresult_t result = NFD_OpenDialog("dsy", NULL, &outPath);

				if (result == NFD_OKAY) {
					if (!loadSettingsFromFile(&scePadSettings, outPath))
						showLoadFailedError = true;

					free(outPath);
				}
			}

			if (ImGui::MenuItem(str("SetDefaultConfig"))) {
				std::string pathToDSYSaves = sago::getDocumentsFolder() + "/DSY/DefaultConfigs/";
				if (!std::filesystem::is_directory(pathToDSYSaves))
					std::filesystem::create_directories(pathToDSYSaves);

				nfdchar_t* outPath = NULL;
				nfdresult_t result = NFD_OpenDialog("dsy", NULL, &outPath);
				s_scePadSettings tempSettings = {};

				if (result == NFD_OKAY) {
					if (!loadSettingsFromFile(&tempSettings, outPath))
						showLoadFailedError = true;

					std::string macAddress = scePadGetMacAddress(g_scePad[currentController]);

					if (macAddress == "")
						showControllerNotConnectedError = true;

					if (!showLoadFailedError && !showControllerNotConnectedError) {
						std::string cleanMac = macAddress;
						cleanMac.erase(std::remove(cleanMac.begin(), cleanMac.end(), ':'), cleanMac.end());
						std::filesystem::path filePath = std::filesystem::path(pathToDSYSaves) / cleanMac;
						std::ofstream file(filePath);
						file << outPath;
						file.close();
					}

					free(outPath);
				}
			}

			if (ImGui::MenuItem(str("RemoveDefaultConfig"))) {
				std::string macAddress = scePadGetMacAddress(g_scePad[currentController]);

				if (macAddress != "") {
					removeDefaultConfigByMac(macAddress);
				}
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(str("Settings"))) {
			if (ImGui::MenuItem(str("DisconnectAllBTDevicesOnExit"), NULL, &m_appSettings.DisableAllBluetoothControllersOnExit))
				saveAppSettings(&m_appSettings);
			if (ImGui::MenuItem(str("DontConnectToServerOnStart"), NULL, &m_appSettings.DontConnectToServerOnStart))
				saveAppSettings(&m_appSettings);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(str("Help"))) {
			ImGui::TextLinkOpenURL("Discord", "https://discord.gg/AFYvxf282U");
			ImGui::TextLinkOpenURL("GitHub", "https://github.com/WujekFoliarz/DualSenseY-v2/issues");
			ImGui::MenuItem(str("About"), "", &openAbout);

			ImGui::EndMenu();
		}

		float textWidth = ImGui::CalcTextSize(std::string(strr("UDPStatus") + ":" + strr("Inactive")).c_str()).x + 10;
		ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - textWidth);
		ImGui::Text(std::string(strr("UDPStatus") + ":").c_str());
		if (m_udp.isActive())
			ImGui::TextColored(ImVec4(0, 1, 0, 1), str("Active"));
		else
			ImGui::TextColored(ImVec4(1, 0, 0, 1), str("Inactive"));

		ImGui::EndMainMenuBar();
	}

	if (openAbout) about(&openAbout);

	return true;
}

bool MainWindow::controllers(int& currentController, s_scePadSettings& scePadSettings, float scale) {
	ImGui::SeparatorText("Controller");

	bool noneConnected = true;
	for (uint32_t i = 0; i < 4; i++) {
		s_ScePadData data = {};
		int result = scePadReadState(g_scePad[i], &data);
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

bool MainWindow::led(s_scePadSettings& scePadSettings, float scale) {
	if (m_udp.isActive())
		return false;

	ImGui::SeparatorText(str("LedSection"));

	ImGui::Checkbox(str("DisablePlayerLED"), &scePadSettings.disablePlayerLed);
	ImGui::Checkbox(str("AudioToLED"), &scePadSettings.audioToLed);
	ImGui::Checkbox(str("DiscoMode"), &scePadSettings.discoMode);
	if (scePadSettings.discoMode) {
		ImGui::SameLine();
		ImGui::SetNextItemWidth(300);
		ImGui::SliderFloat(str("Speed"), &scePadSettings.discoModeSpeed, 0.020, 2.0);
	}

	ImGui::Text(str("PlayerLedBrightness")); ImGui::SameLine();
	ImGui::RadioButton(str("High"), &scePadSettings.brightness, 0); ImGui::SameLine();
	ImGui::RadioButton(str("Medium"), &scePadSettings.brightness, 1); ImGui::SameLine();
	ImGui::RadioButton(str("Low"), &scePadSettings.brightness, 2);

	if (ImGui::TreeNode(str("ColorPicker"))) {
		ImGui::SetNextItemWidth(scale);
		ImGui::ColorPicker3(str("LightbarColor"), scePadSettings.led.data(), ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
		ImGui::TreePop();
	}

	return true;
}

bool MainWindow::audio(int currentController, s_scePadSettings& scePadSettings) {
	static bool failedToStart = false;
	int busType = 0;
	scePadGetControllerBusType(g_scePad[currentController], &busType);

	ImGui::SeparatorText(str("Audio"));

	if (busType == SCE_PAD_BUSTYPE_BT) {
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "Bluetooth");
		return true;
	}

	static bool wasChecked = false;
	ImGui::Checkbox(str("Audio passthrough"), &scePadSettings.audioPassthrough);

	if (!scePadSettings.audioPassthrough && wasChecked) {
		wasChecked = false;
		if (!m_audio.stopByUserId(currentController + 1)) {
			LOGE("Failed to stop audio passthrough");
		}
	}

	if (scePadSettings.audioPassthrough && !wasChecked) {
		wasChecked = true;
		if (!m_audio.startByUserId(currentController + 1)) {
			LOGE("Failed to start audio passthrough");
			scePadSettings.audioPassthrough = false;
			failedToStart = true;
		}
		else {
			failedToStart = false;
		}
	}

	if (failedToStart) {
		ImGui::SameLine();
	#ifdef WINDOWS
		ImGui::TextColored(ImVec4(1, 0, 0, 1), str("Failed to start"));
	#else
		ImGui::TextColored(ImVec4(1, 0, 0, 1), str("Audio passthrough is not available on this platform"));
	#endif
	}
	else if (!failedToStart && scePadSettings.audioPassthrough) {
		ImGui::SetNextItemWidth(400);
		ImGui::SliderFloat(str("HapticsIntensity"), &scePadSettings.hapticIntensity, 0.0f, 5.0f);
	}

	if (ImGui::TreeNode(str("AudioOutputPath"))) {
		ImGui::RadioButton(str("StereoHeadset"), &scePadSettings.audioPath, SCE_PAD_AUDIO_PATH_STEREO_HEADSET);
		ImGui::RadioButton(str("MonoLeftHeadset"), &scePadSettings.audioPath, SCE_PAD_AUDIO_PATH_MONO_LEFT_HEADSET);
		ImGui::RadioButton(str("MonoLeftHeadsetAndSpeaker"), &scePadSettings.audioPath, SCE_PAD_AUDIO_PATH_MONO_LEFT_HEADSET_AND_SPEAKER);
		ImGui::RadioButton(str("OnlySpeaker"), &scePadSettings.audioPath, SCE_PAD_AUDIO_PATH_ONLY_SPEAKER);
		ImGui::TreePop();
	}

	ImGui::SetNextItemWidth(400);
	ImGui::SliderInt(str("SpeakerVolume"), &scePadSettings.speakerVolume, 0, 8, "%d");
	ImGui::SetNextItemWidth(400);
	ImGui::SliderInt(str("MicrophoneGain"), &scePadSettings.micGain, 0, 8, "%d");
	return true;
}

static std::vector<std::string> sonyItems = { TriggerStringSony::OFF, TriggerStringSony::FEEDBACK, TriggerStringSony::WEAPON, TriggerStringSony::VIBRATION, TriggerStringSony::SLOPE_FEEDBACK, TriggerStringSony::MULTIPLE_POSITION_FEEDBACK, TriggerStringSony::MULTIPLE_POSITION_VIBRATION };
static std::vector<std::string> dsxItems = { TriggerStringDSX::Normal, TriggerStringDSX::GameCube, TriggerStringDSX::VerySoft, TriggerStringDSX::Soft, TriggerStringDSX::Medium, TriggerStringDSX::Hard, TriggerStringDSX::VeryHard , TriggerStringDSX::Hardest, TriggerStringDSX::VibrateTrigger, TriggerStringDSX::VibrateTriggerPulse, TriggerStringDSX::Choppy, TriggerStringDSX::CustomTriggerValue, TriggerStringDSX::Resistance,TriggerStringDSX::Bow,TriggerStringDSX::Galloping,TriggerStringDSX::SemiAutomaticGun, TriggerStringDSX::AutomaticGun, TriggerStringDSX::Machine, TriggerStringDSX::VIBRATE_TRIGGER_10Hz };
bool MainWindow::adaptiveTriggers(s_scePadSettings& scePadSettings) {
	if (m_udp.isActive())
		return false;

	ImGui::SeparatorText(str("AdaptiveTriggers"));

	if (ImGui::TreeNodeEx(str("StaticTriggerSettings"))) {
		ImGui::Text(str("SelectedTrigger"));
		ImGui::RadioButton("L2", &scePadSettings.uiSelectedTrigger, L2); ImGui::SameLine();
		ImGui::RadioButton("R2", &scePadSettings.uiSelectedTrigger, R2);

		ImGui::Text(str("TriggerFormat"));
		ImGui::RadioButton("Sony", &scePadSettings.uiTriggerFormat[scePadSettings.uiSelectedTrigger], SONY_FORMAT); ImGui::SameLine();
		ImGui::RadioButton("DSX", &scePadSettings.uiTriggerFormat[scePadSettings.uiSelectedTrigger], DSX_FORMAT);

		int currentlySelectedTrigger = scePadSettings.uiSelectedTrigger;
		int currentTriggerFormat = scePadSettings.uiTriggerFormat[currentlySelectedTrigger];

		ImGui::SetNextItemWidth(450);
		if (ImGui::BeginCombo(str("TriggerMode"), currentTriggerFormat == SONY_FORMAT ? scePadSettings.uiSelectedSonyTriggerMode[currentlySelectedTrigger].c_str()
			: scePadSettings.uiSelectedDSXTriggerMode[currentlySelectedTrigger].c_str())) {
			std::vector<std::string>& items = (currentTriggerFormat == SONY_FORMAT) ? sonyItems : dsxItems;
			int& currentItem = (currentTriggerFormat == SONY_FORMAT) ? scePadSettings.currentSonyItem[currentlySelectedTrigger] : scePadSettings.currentDSXItem[currentlySelectedTrigger];

			for (int i = 0; i < items.size(); i++) {
				bool isSelected = (currentItem == i);
				if (ImGui::Selectable(items[i].c_str(), isSelected)) {
					currentItem = i;
				}
			}
			ImGui::EndCombo();
		}

		if (currentTriggerFormat == SONY_FORMAT) {
			scePadSettings.isLeftUsingDsxTrigger = currentlySelectedTrigger == L2 ? false : scePadSettings.isLeftUsingDsxTrigger;
			scePadSettings.isRightUsingDsxTrigger = currentlySelectedTrigger == R2 ? false : scePadSettings.isRightUsingDsxTrigger;
			scePadSettings.uiSelectedSonyTriggerMode[currentlySelectedTrigger] = sonyItems[scePadSettings.currentSonyItem[currentlySelectedTrigger]];
		}
		else {
			scePadSettings.isLeftUsingDsxTrigger = currentlySelectedTrigger == L2 ? true : scePadSettings.isLeftUsingDsxTrigger;
			scePadSettings.isRightUsingDsxTrigger = currentlySelectedTrigger == R2 ? true : scePadSettings.isRightUsingDsxTrigger;
			scePadSettings.uiSelectedDSXTriggerMode[currentlySelectedTrigger] = dsxItems[scePadSettings.currentDSXItem[currentlySelectedTrigger]];
		}

		if (scePadSettings.uiTriggerFormat[currentlySelectedTrigger] == SONY_FORMAT) {
			if (scePadSettings.uiSelectedSonyTriggerMode[currentlySelectedTrigger] == TriggerStringSony::FEEDBACK) {
				int& position = scePadSettings.uiParameters[currentlySelectedTrigger][0];
				int& strength = scePadSettings.uiParameters[currentlySelectedTrigger][1];

				if (position > 9) position = 9;
				if (strength < 1) strength = 1;
				if (strength > 8) strength = 8;

				ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("Position"), &position, 0, 9); ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("Strength"), &strength, 1, 8);
			}
			else if (scePadSettings.uiSelectedSonyTriggerMode[currentlySelectedTrigger] == TriggerStringSony::WEAPON) {
				int& startPosition = scePadSettings.uiParameters[currentlySelectedTrigger][0];
				int& endPosition = scePadSettings.uiParameters[currentlySelectedTrigger][1];
				int& strength = scePadSettings.uiParameters[currentlySelectedTrigger][2];

				if (startPosition < 2) startPosition = 2;
				if (startPosition > 7) startPosition = 7;
				if (endPosition > 8) endPosition = 8;
				if (strength < 1) strength = 1;
				if (strength > 8) strength = 8;

				ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("StartPosition"), &startPosition, 2, 7); ImGui::SetNextItemWidth(450);
				if (startPosition >= endPosition)
					endPosition = startPosition + 1;
				ImGui::SliderInt(str("EndPosition"), &endPosition, scePadSettings.uiParameters[currentlySelectedTrigger][0] + 1, 8); ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("Strength"), &strength, 1, 8);
			}
			else if (scePadSettings.uiSelectedSonyTriggerMode[currentlySelectedTrigger] == TriggerStringSony::VIBRATION) {
				int& position = scePadSettings.uiParameters[currentlySelectedTrigger][0];
				int& amplitude = scePadSettings.uiParameters[currentlySelectedTrigger][1];
				int& frequency = scePadSettings.uiParameters[currentlySelectedTrigger][2];

				if (position > 9) position = 9;
				if (amplitude < 1) amplitude = 1;
				if (amplitude > 8) amplitude = 8;
				if (frequency < 1) frequency = 1;

				ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("Position"), &position, 0, 9); ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("Amplitude"), &amplitude, 1, 8); ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("Frequency"), &frequency, 1, 255);
			}
			else if (scePadSettings.uiSelectedSonyTriggerMode[currentlySelectedTrigger] == TriggerStringSony::SLOPE_FEEDBACK) {
				int& startPosition = scePadSettings.uiParameters[currentlySelectedTrigger][0];
				int& endPosition = scePadSettings.uiParameters[currentlySelectedTrigger][1];
				int& startStrength = scePadSettings.uiParameters[currentlySelectedTrigger][2];
				int& endStrength = scePadSettings.uiParameters[currentlySelectedTrigger][3];

				if (startPosition < 1) startPosition = 1;
				if (startPosition > 8) startPosition = 8;
				if (endPosition <= startPosition) endPosition = startPosition + 1;
				if (endPosition > 9) endPosition = 9;
				if (startStrength > 8) startStrength = 8;
				if (startStrength < 1) startStrength = 1;
				if (endStrength < 1) endStrength = 1;
				if (endStrength > 8) endStrength = 8;

				ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("StartPosition"), &startPosition, 1, endPosition - 1); ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("EndPosition"), &endPosition, startPosition + 1, 9); ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("StartStrength"), &startStrength, 1, 8); ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("EndStrength"), &endStrength, 1, 8);
			}
			else if (scePadSettings.uiSelectedSonyTriggerMode[currentlySelectedTrigger] == TriggerStringSony::MULTIPLE_POSITION_FEEDBACK) {
				std::string strengthStr = str("Strength");
				for (int i = 0; i < 10; ++i) {
					if (scePadSettings.uiParameters[currentlySelectedTrigger][i] > 8) scePadSettings.uiParameters[currentlySelectedTrigger][i] = 8;
					ImGui::SetNextItemWidth(450);
					ImGui::SliderInt(std::string(strengthStr + " " + std::to_string(i + 1)).c_str(), &scePadSettings.uiParameters[currentlySelectedTrigger][i], 0, 8);
				}
			}
			else if (scePadSettings.uiSelectedSonyTriggerMode[currentlySelectedTrigger] == TriggerStringSony::MULTIPLE_POSITION_VIBRATION) {
				std::string amplitudeStr = str("Amplitude");
				int& frequency = scePadSettings.uiParameters[currentlySelectedTrigger][0];

				ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(amplitudeStr.c_str(), &frequency, 0, 255);
				for (int i = 1; i < 11; ++i) {
					if (scePadSettings.uiParameters[currentlySelectedTrigger][i] > 8) scePadSettings.uiParameters[currentlySelectedTrigger][i] = 8;
					ImGui::SetNextItemWidth(450);
					ImGui::SliderInt(std::string(amplitudeStr + " " + std::to_string(i)).c_str(), &scePadSettings.uiParameters[currentlySelectedTrigger][i], 0, 8);
				}
			}
		}
		else {
			if (scePadSettings.uiSelectedDSXTriggerMode[currentlySelectedTrigger] == TriggerStringDSX::CustomTriggerValue) {
				static const std::vector<std::string> customTriggerList = { "Off", "Rigid", "Rigid_A", "Rigid_B", "Rigid_AB", "Pulse", "Pulse_A", "Pulse_B", "Pulse_AB" };
				int& currentlySelectedCustomTrigger = scePadSettings.uiParameters[currentlySelectedTrigger][0];
				if (currentlySelectedCustomTrigger > customTriggerList.size()) currentlySelectedCustomTrigger = customTriggerList.size() - 1;

				ImGui::SetNextItemWidth(450);
				if (ImGui::BeginCombo(str("CustomTriggerMode"), customTriggerList[currentlySelectedCustomTrigger].c_str())) {
					for (int i = 0; i < customTriggerList.size(); i++) {
						bool isSelected = (currentlySelectedCustomTrigger == i);
						if (ImGui::Selectable(customTriggerList[i].c_str(), isSelected)) {
							currentlySelectedCustomTrigger = i;
						}
					}

					ImGui::EndCombo();
				}

				std::string paramStr = str("Parameter");
				for (int i = 1; i < MAX_PARAM_COUNT; i++) {
					ImGui::SetNextItemWidth(450);
					ImGui::SliderInt(std::string(paramStr + " " + std::to_string(i)).c_str(), &scePadSettings.uiParameters[currentlySelectedTrigger][i], 0, 255);
				}
			}
			else if (scePadSettings.uiSelectedDSXTriggerMode[currentlySelectedTrigger] == TriggerStringDSX::Resistance) {
				int& start = scePadSettings.uiParameters[currentlySelectedTrigger][0];
				int& force = scePadSettings.uiParameters[currentlySelectedTrigger][1];

				if (start > 9) start = 9;
				if (force > 8) force = 8;

				ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("Start"), &start, 0, 9); ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("Force"), &force, 0, 8);
			}
			else if (scePadSettings.uiSelectedDSXTriggerMode[currentlySelectedTrigger] == TriggerStringDSX::Bow) {
				int& start = scePadSettings.uiParameters[currentlySelectedTrigger][0];
				int& end = scePadSettings.uiParameters[currentlySelectedTrigger][1];
				int& force = scePadSettings.uiParameters[currentlySelectedTrigger][2];
				int& snapForce = scePadSettings.uiParameters[currentlySelectedTrigger][3];

				if (start > 8) start = 8;
				if (start >= end) end = start + 1;
				if (end > 8) end = 8;
				if (force > 8) force = 8;
				if (snapForce > 8) snapForce = 8;

				ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("Start"), &start, 0, 7); ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("End"), &end, start + 1, 8); ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("Force"), &force, 0, 8); ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("SnapForce"), &snapForce, 0, 8);
			}
			else if (scePadSettings.uiSelectedDSXTriggerMode[currentlySelectedTrigger] == TriggerStringDSX::Galloping) {
				int& start = scePadSettings.uiParameters[currentlySelectedTrigger][0];
				int& end = scePadSettings.uiParameters[currentlySelectedTrigger][1];
				int& firstFoot = scePadSettings.uiParameters[currentlySelectedTrigger][2];
				int& secondFoot = scePadSettings.uiParameters[currentlySelectedTrigger][3];
				int& frequency = scePadSettings.uiParameters[currentlySelectedTrigger][4];

				if (start > 8) start = 8;
				if (end > 9) end = 9;
				if (firstFoot > 7) firstFoot = 7;
				if (secondFoot > 6) secondFoot = 6;
				if (frequency < 1) frequency = 1;

				ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("Start"), &start, 0, end - 1); ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("End"), &end, start, 9); ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("FirstFoot"), &firstFoot, 0, secondFoot); ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("SecondFoot"), &secondFoot, firstFoot, 6); ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("Frequency"), &frequency, 0, 255);
			}
			else if (scePadSettings.uiSelectedDSXTriggerMode[currentlySelectedTrigger] == TriggerStringDSX::SemiAutomaticGun) {
				int& start = scePadSettings.uiParameters[currentlySelectedTrigger][0];
				int& end = scePadSettings.uiParameters[currentlySelectedTrigger][1];
				int& force = scePadSettings.uiParameters[currentlySelectedTrigger][2];

				if (start < 2) start = 2;
				if (start > 7) start = 7;
				if (end > 8) end = 8;
				if (end < start) end = start + 1;
				if (force > 8) force = 8;
				if (force < 1) force = 1;

				ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("Start"), &start, 2, end - 1); ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("End"), &end, start, 8); ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("Force"), &force, 1, 8);
			}
			else if (scePadSettings.uiSelectedDSXTriggerMode[currentlySelectedTrigger] == TriggerStringDSX::AutomaticGun) {
				int& start = scePadSettings.uiParameters[currentlySelectedTrigger][0];
				int& strength = scePadSettings.uiParameters[currentlySelectedTrigger][1];
				int& frequency = scePadSettings.uiParameters[currentlySelectedTrigger][2];

				if (start > 9) start = 9;
				if (strength > 8) strength = 8;
				if (strength < 1) strength = 1;
				if (frequency < 1) frequency = 1;

				ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("Start"), &start, 0, 9); ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("Strength"), &strength, 1, 8); ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("Frequency"), &frequency, 1, 255);
			}
			else if (scePadSettings.uiSelectedDSXTriggerMode[currentlySelectedTrigger] == TriggerStringDSX::Machine) {
				int& start = scePadSettings.uiParameters[currentlySelectedTrigger][0];
				int& end = scePadSettings.uiParameters[currentlySelectedTrigger][1];
				int& strengthA = scePadSettings.uiParameters[currentlySelectedTrigger][2];
				int& strengthB = scePadSettings.uiParameters[currentlySelectedTrigger][3];
				int& frequency = scePadSettings.uiParameters[currentlySelectedTrigger][4];
				int& period = scePadSettings.uiParameters[currentlySelectedTrigger][5];

				if (start > 8) start = 8;
				if (end > 9) end = 9;
				if (end < start) end = start + 1;
				if (strengthA > 7) strengthA = 7;
				if (strengthB > 7) strengthB = 7;
				if (frequency < 1) frequency = 7;

				std::string strStrength = str("Strength");
				ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("Start"), &start, 0, end - 1); ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("End"), &end, start, 9); ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(std::string(strStrength + " A").c_str(), &strengthA, 0, 7); ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(std::string(strStrength + " B").c_str(), &strengthB, 0, 7); ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("Frequency"), &frequency, 0, 255); ImGui::SetNextItemWidth(450);
				ImGui::SliderInt(str("Period"), &period, 0, 255);
			}
		}

		ImGui::TreePop();
	}
	return true;
}

bool MainWindow::keyboardAndMouseMapping(s_scePadSettings& scePadSettings) {
	ImGui::SeparatorText(str("KeyboardAndMouseMapping"));
	ImGui::Checkbox("Analog WSAD emulation", &scePadSettings.emulateAnalogWsad);
	return true;
}

bool MainWindow::touchpad(int currentController, s_scePadSettings& scePadSettings, s_ScePadData& state, float scale) {
	ImGui::SeparatorText(str("Touchpad"));

	ImGui::Checkbox(str("TouchpadToMouse"), &scePadSettings.touchpadAsMouse);
	if (scePadSettings.touchpadAsMouse) {
		ImGui::SetNextItemWidth(400);
		ImGui::SliderFloat(str("Sensitivity"), &scePadSettings.touchpadAsMouse_sensitivity, 0.0f, 5.0f);
	}
	treeElement_touchpadDiagnostics(currentController, scePadSettings, state, scale);

	return true;
}

bool MainWindow::treeElement_touchpadDiagnostics(int currentController, s_scePadSettings& scePadSettings, s_ScePadData& state, float scale) {

	if (ImGui::TreeNodeEx(str("Diagnostics"))) {
		ImVec2 touchpadSize(
	1.160 * scale,
	0.520 * scale);
		ImGui::InvisibleButton("##touchpad_bg", touchpadSize);
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 touchpadPos = ImGui::GetItemRectMin();
		drawList->AddRectFilled(touchpadPos,
								ImVec2(touchpadPos.x + touchpadSize.x,
								touchpadPos.y + touchpadSize.y),
								IM_COL32(state.bitmask_buttons & SCE_BM_TOUCH ? 200 : 50, 50, 50, 255));

		s_ScePadInfo info = {};
		scePadGetControllerInformation(g_scePad[currentController], &info);

		auto drawFinger = [&](float x, float y, int id, bool notTouching) {
			if (!notTouching) {
				float scaledX = touchpadPos.x + (x / (float)info.touchPadInfo.resolution.x) * touchpadSize.x;
				float scaledY = touchpadPos.y + (y / (float)info.touchPadInfo.resolution.y) * touchpadSize.y;
				drawList->AddCircleFilled(ImVec2(scaledX, scaledY), 0.02f * scale, IM_COL32(255, 0, 0, 255));

				ImGui::GetWindowDrawList()->AddText(ImVec2(scaledX - 20, scaledY), IM_COL32(255, 255, 255, 255), std::to_string(id).c_str());
				ImGui::GetWindowDrawList()->AddText(ImVec2(scaledX - 50, scaledY - 38), IM_COL32(255, 255, 255, 255), std::string(std::to_string((int)x) + "," + std::to_string((int)y)).c_str());
			}
			};

		drawFinger((float)state.touchData.touch[0].x, (float)state.touchData.touch[0].y, state.touchData.touch[0].id, state.touchData.touch[0].reserve[0]);
		drawFinger((float)state.touchData.touch[1].x, (float)state.touchData.touch[1].y, state.touchData.touch[1].id, state.touchData.touch[1].reserve[0]);

		ImGui::TreePop();
	}

	return true;
}

bool MainWindow::treeElement_lightbar(s_scePadSettings& scePadSettings) {
	if (ImGui::TreeNodeEx(str("Lightbar"))) {
		ImGui::Checkbox(str("UseEmulatedLightbar"), &scePadSettings.useLightbarFromEmulatedController);
		ImGui::TreePop();
	}

	return true;
}

bool MainWindow::treeElement_vibration(s_scePadSettings& scePadSettings) {
	if (ImGui::TreeNodeEx(str("Vibration"))) {
		ImGui::Checkbox(str("UseEmulatedVibration"), &scePadSettings.useRumbleFromEmulatedController);
		ImGui::TreePop();
	}

	return true;
}

bool MainWindow::treeElement_dynamicAdaptiveTriggers(s_scePadSettings& scePadSettings) {
	if (ImGui::TreeNodeEx(str("DynamicTriggerSettings"))) {
		ImGui::Checkbox(str("RumbleToAT"), &scePadSettings.rumbleToAT);
		if (scePadSettings.rumbleToAT) {
			ImGui::Checkbox(str("SwapTriggersRumbleToAT"), &scePadSettings.rumbleToAt_swapTriggers);
		}

		static int selectedTrigger = SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2;
		auto rumbleToAtSetting = [&](int& selectedTrigger) {
			ImGui::SetNextItemWidth(400);
			ImGui::SliderInt(str("MaxFrequency"), &scePadSettings.rumbleToAt_frequency[selectedTrigger], 0, 255);
			ImGui::SetNextItemWidth(400);
			ImGui::SliderInt(str("MaxIntensity"), &scePadSettings.rumbleToAt_intensity[selectedTrigger], 0, 255);
			ImGui::SetNextItemWidth(400);
			ImGui::SliderInt(str("Position"), &scePadSettings.rumbleToAt_position[selectedTrigger], 0, 139);
			};

		ImGui::RadioButton("L2", &selectedTrigger, 0);
		ImGui::SameLine();
		ImGui::RadioButton("R2", &selectedTrigger, 1);
		rumbleToAtSetting(selectedTrigger);

		ImGui::TreePop();
	}
	return true;
}

bool MainWindow::online() {
	static bool showMsgFromServer = false;
	SCMD::CMD_CODE_RESPONSE currentResponse = m_client.GetLastResponseInQueue();

	if (!m_client.IsResponseQueueEmpty() && currentResponse.Code != RESPONSE_CODE::E_SUCCESS) {
		showMsgFromServer = true;

		if (messageFromServer(&showMsgFromServer, &currentResponse)) {
			m_client.PopBackResponseQueue();
		}
	}
	else {
		showMsgFromServer = false;
	}

	bool fetchingData = m_client.IsFetchingDataFromServer();
	screenBlock(&fetchingData, str("FetchingFromServer"));

	fetchingData = m_client.IsFetchingDataFromPeer();
	screenBlock(&fetchingData, str("FetchingFromPeer"));

	ImGui::SeparatorText(str("Online"));

	if (m_client.IsConnectionOccupied()) {
		ImGui::Text(str("FailedToCreateHost"));
		return false;
	}

	if (!m_client.IsConnected()) {
		if (m_client.IsConnecting())
			ImGui::Text(str("ConnectingToServer"));
		else if (ImGui::Button(str("ConnectOnline")))
			m_client.Connect();
	}
	else {

		ImGui::Text("%s: %d", str("UsersOnline"), (int)m_client.GetGlobalPeerCount());

		if (!m_client.IsInRoom()) {
			static char buf[MAX_ROOM_NAME_SIZE] = { 0 };
			ImGui::SetNextItemWidth(250);
			ImGui::InputText(str("RoomName"), buf, MAX_ROOM_NAME_SIZE);
			std::string roomName = std::string(buf, strnlen(buf, MAX_ROOM_NAME_SIZE));

			if (ImGui::Button(str("CreateRoom"))) {
				m_client.CMD_OPEN_ROOM(roomName.c_str());
			}
			ImGui::SameLine();
			if (ImGui::Button(str("JoinRoom"))) {
				m_client.CMD_JOIN_ROOM(roomName.c_str());
			}

		}
		else {
			static bool showRoomName = false;
			if (showRoomName)
				ImGui::Text("%s: %s", str("Room"), m_client.GetRoomName().c_str());
			else if (ImGui::Button(str("ShowRoomName")))
				showRoomName = true;

			ImGui::SameLine();
			if (ImGui::Button(str("LeaveRoom"))) {
				m_client.CMD_LEAVE_ROOM();
			}

			auto peers = m_client.GetPeerList();
			if (!peers.empty()) {
				for (auto& peer : peers) {
					ImGui::Text("[%d] %s - %d ms", peer.first, peer.second.c_str(), m_client.GetPingFromPeer(peer.first));
					ImGui::SameLine();

					auto requestStatus = m_client.GetRequestStatus(peer.first);

					if (requestStatus == PEER_REQUEST_STATUS::PEER_WAITING_FOR_MY_RESPONSE) {
						ImGui::Text("[%s]", str("NewControllerRequest"));
						ImGui::SameLine();
						if (ImGui::SmallButton(str("Accept"))) {
							m_client.AcceptPeerRequest(peer.first);
						}
						ImGui::SameLine();
						if (ImGui::SmallButton(str("Decline"))) {
							m_client.DeclinePeerRequest(peer.first);
						}
					}
					else if (requestStatus == PEER_REQUEST_STATUS::WAITING_FOR_PEER_RESPONSE) {
						ImGui::Text(str("WaitingForPeerResponse"));
					}
					else if (requestStatus == PEER_REQUEST_STATUS::ME_TRANSMITTING_TO_PEER) {
						ImGui::TextColored(ImVec4(0, 1, 0, 1), str("TransmitingToPeer"));
						ImGui::SameLine();
						if (ImGui::SmallButton(str("Abort")))
							m_client.CMD_PEER_ABORT_VIGEM(peer.first);
					}
					else if (requestStatus == PEER_REQUEST_STATUS::PEER_TRANSMITING_TO_ME) {
						ImGui::TextColored(ImVec4(0, 1, 0, 1), str("PeerTransmitingToYou"));
						ImGui::SameLine();
						if (ImGui::SmallButton(str("Abort")))
							m_client.CMD_PEER_ABORT_VIGEM(peer.first);
					}
					else {
						if (requestStatus == PEER_REQUEST_STATUS::PEER_DECLINED) {
							ImGui::TextColored(ImVec4(1, 0, 0, 1), str("PeerDeclined"));
						}
						ImGui::SameLine();

						if (ImGui::SmallButton(str("RequestX360"))) {
							m_client.CMD_PEER_REQUEST_VIGEM(peer.first, CONTROLLER::XBOX360);
						}
						ImGui::SameLine();
						if (ImGui::SmallButton(str("RequestDS4"))) {
							m_client.CMD_PEER_REQUEST_VIGEM(peer.first, CONTROLLER::DUALSHOCK4);
						}
					}
				}
			}
			else {
				ImGui::Text(str("AwaitingForPeerToJoin"));
			}
		}
	}

	return true;
}

bool MainWindow::messageFromServer(bool* open, SCMD::CMD_CODE_RESPONSE* Response) {

	if (open) {
		ImGui::OpenPopup(str("MessageFromServer"));
	}

	std::string message = strr("UnknownResponse");

	if (Response->Cmd == CMD::CMD_OPEN_ROOM && Response->Code == RESPONSE_CODE::E_ROOM_ALREADY_EXISTS) {
		message = strr("ThisRoomAlreadyExists");
	}
	else if (Response->Cmd == CMD::CMD_OPEN_ROOM && Response->Code == RESPONSE_CODE::E_PEER_ALREADY_IN_ROOM) {
		message = strr("YoureAlreadyInARoom");
	}
	else if (Response->Cmd == CMD::CMD_OPEN_ROOM && Response->Code == RESPONSE_CODE::E_ROOM_NAME_EMPTY) {
		message = strr("TheRoomNameIsEmpty");
	}

	if (Response->Cmd == CMD::CMD_JOIN_ROOM && Response->Code == RESPONSE_CODE::E_ROOM_FULL) {
		message = strr("ThisRoomIsFull");
	}
	else if (Response->Cmd == CMD::CMD_JOIN_ROOM && Response->Code == RESPONSE_CODE::E_ROOM_DOESNT_EXIST) {
		message = strr("ThisRoomDoesntExist");
	}
	else if (Response->Cmd == CMD::CMD_JOIN_ROOM && Response->Code == RESPONSE_CODE::E_ROOM_NAME_EMPTY) {
		message = strr("TheRoomNameIsEmpty");
	}

	if (Response->Cmd == CMD::CMD_PEER_REQUEST_VIGEM && Response->Code == RESPONSE_CODE::E_PEER_CANT_EMULATE) {
		message = strr("PeerNoVigem");
	}
	else if (Response->Cmd == CMD::CMD_PEER_REQUEST_VIGEM && Response->Code == RESPONSE_CODE::E_PEER_DECLINE) {
		message = strr("PeerDeclinedRequest");
	}

	bool clicked = false;
	if (ImGui::BeginPopupModal(str("MessageFromServer"), open, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text(message.c_str());
		if (ImGui::Button("OK")) {
			*open = false;
			ImGui::CloseCurrentPopup();
			clicked = true;
		}
		ImGui::EndPopup();
	}

	return clicked;
}

bool MainWindow::screenBlock(bool* open, const char* Message) {
	if (*open) {
		ImGui::OpenPopup("Block");

		if (ImGui::BeginPopupModal("Block", open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs)) {
			ImGui::Text(Message);
			ImGui::EndPopup();
		}
	}
	return true;
}

void MainWindow::errors() {
	if (showLoadFailedError) {
		ImGui::OpenPopup(str("Error"));
	}

	if (ImGui::BeginPopupModal(str("Error"), &showLoadFailedError, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text(str("ErrorLoadConfig"));
		ImGui::Separator();

		if (ImGui::Button("OK", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
			showLoadFailedError = false;
		}

		ImGui::EndPopup();
	}
}

bool MainWindow::treeElement_analogSticks(s_scePadSettings& scePadSettings, s_ScePadData& state) {
	if (ImGui::TreeNodeEx(str("AnalogSticks"))) {
		const int previewSize = 100;
		const ImU32 whiteColor = IM_COL32(255, 255, 255, 255);
		const ImU32 redColor = IM_COL32(255, 0, 0, 255);
		const ImU32 greenColor = IM_COL32(0, 255, 0, 255);

		auto drawStick = [](const s_SceStickData& stick, bool isPressed, int deadzone, ImVec2 centerPos) {
			const float radius = static_cast<float>(previewSize);
			ImGui::GetWindowDrawList()->AddCircle(centerPos, radius, isPressed ? redColor : whiteColor, 32, 2.0f);
			float normDeadzone = (deadzone * radius) / 128;
			ImGui::GetWindowDrawList()->AddCircle(centerPos, normDeadzone, greenColor, 32, 2.0f);

			float normX = (stick.X - 128) / 127.0f;
			float normY = -((stick.Y - 128) / 127.0f);

			ImVec2 stickPos = centerPos;
			stickPos.x += normX * radius;
			stickPos.y -= normY * radius;

			ImGui::GetWindowDrawList()->AddCircleFilled(stickPos, 5, redColor, 32);
			ImGui::GetWindowDrawList()->AddText(ImVec2(stickPos.x, stickPos.y), whiteColor, std::to_string(stick.X).c_str());
			ImGui::GetWindowDrawList()->AddText(ImVec2(stickPos.x - 19, stickPos.y - 40), whiteColor, std::to_string(stick.Y).c_str());
			};

		ImVec2 leftCenter = ImGui::GetCursorScreenPos();
		leftCenter.x += previewSize;
		leftCenter.y += previewSize;

		drawStick(state.LeftStick, state.bitmask_buttons & SCE_BM_L3 ? true : false, scePadSettings.leftStickDeadzone, leftCenter);

		ImVec2 rightCenter = leftCenter;
		rightCenter.x += previewSize * 2.1f;

		drawStick(state.RightStick, state.bitmask_buttons & SCE_BM_R3 ? true : false, scePadSettings.rightStickDeadzone, rightCenter);

		ImGui::Dummy(ImVec2(1, previewSize * 2));
		ImGui::SetNextItemWidth(400);
		ImGui::SliderInt(str("LeftAnalogStickDeadZone"), &scePadSettings.leftStickDeadzone, 0, 127);
		ImGui::SetNextItemWidth(400);
		ImGui::SliderInt(str("RightAnalogStickDeadZone"), &scePadSettings.rightStickDeadzone, 0, 127);
		ImGui::TreePop();
	}

	return true;
}

bool MainWindow::emulation(int currentController, s_scePadSettings& scePadSettings, s_ScePadData& state) {
	ImGui::SeparatorText(str("EmulationHeader"));

	if (!m_vigem.isVigemConnected()) {
	#if (!defined(__linux__)) && (!defined(__MACOS__))
		ImGui::TextColored(ImVec4(1, 0, 0, 1), str("VigemMissing")); ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();
		ImGui::TextLinkOpenURL(str("VigemInstallLink"), "https://github.com/nefarius/ViGEmBus/releases/download/v1.22.0/ViGEmBus_1.22.0_x64_x86_arm64.exe");
	#else
		ImGui::TextColored(ImVec4(1, 0, 0, 1), str("VigemNotAvailablePlatform"));
	#endif
	}
	else {
		ImGui::RadioButton(str("None"), &scePadSettings.emulatedController, 0); ImGui::SameLine();
		ImGui::RadioButton("Xbox 360", &scePadSettings.emulatedController, 1); ImGui::SameLine();
		ImGui::RadioButton("DualShock 4", &scePadSettings.emulatedController, 2);
		m_vigem.plugControllerByIndex(currentController, scePadSettings.emulatedController);

		ImGui::NewLine();
		if (ImGui::TreeNodeEx(str("ControllerSettings"), ImGuiTreeNodeFlags_DefaultOpen)) {

			if (ImGui::TreeNode(str("HideRealController"))) {
				if (m_isAdminWindows) {
					if (ImGui::Button(str("Hide"))) {
						hideController(scePadGetPath(g_scePad[currentController]));
					}
					ImGui::SameLine();
					if (ImGui::Button(str("Unhide"))) {
						unhideController(scePadGetPath(g_scePad[currentController]));
					}
				}
				else {
					ImGui::TextColored(ImVec4(1, 1, 0, 1), str("UnavailableInNonAdminMode"));
				}
				ImGui::TreePop();
			}

			treeElement_analogSticks(scePadSettings, state);
			treeElement_lightbar(scePadSettings);
			treeElement_vibration(scePadSettings);
			treeElement_dynamicAdaptiveTriggers(scePadSettings);

			ImGui::TreePop();
		}
	}

	return true;
}

void MainWindow::show(s_scePadSettings scePadSettings[4], float scale) {
	static int c = 0;
	scale = 100 * (scale * 2.5);

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);

	ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus);
	ImGui::TextColored(ImVec4(1, 0, 0, 1), "Work in progress. Older build at v2 branch on GitHub");
	s_ScePadData state = {};
	scePadReadState(g_scePad[c], &state);

	errors();
	menuBar(c, scePadSettings[c]);
	if (controllers(c, scePadSettings[c], scale)) {
		emulation(c, scePadSettings[c], state);
		led(scePadSettings[c], scale);
		adaptiveTriggers(scePadSettings[c]);
		audio(c, scePadSettings[c]);
		touchpad(c, scePadSettings[c], state, scale);
		keyboardAndMouseMapping(scePadSettings[c]);
	}
	online();

	// Apply triggers from UI
	for (int i = 0; i < TRIGGER_COUNT; i++) {
		std::vector<uint8_t> vec;

		for (int j = 0; j < MAX_PARAM_COUNT; j++) {
			vec.push_back(scePadSettings[c].uiParameters[i][j]);
		}

		if (scePadSettings[c].uiTriggerFormat[i] == SONY_FORMAT) {
			if (auto it = sonyTriggerHandlers.find(sonyItems[scePadSettings[c].currentSonyItem[i]]); it != sonyTriggerHandlers.end())
				it->second(scePadSettings[c], i, vec);
		}
		else {
			if (auto it = dsxTriggerHandlers.find(dsxItems[scePadSettings[c].currentDSXItem[i]]); it != dsxTriggerHandlers.end())
				it->second(scePadSettings[c], i, vec);
		}
	}

	m_selectedController = c;

	ImGui::End();
}

int MainWindow::getSelectedController() {
	return m_selectedController;
}
