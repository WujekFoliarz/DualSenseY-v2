#include "mainWindow.hpp"

#include <imgui.h>
#include <string>

#include <cmath>
#include "log.hpp"
#include <duaLib.h>
#include "scePadHandle.hpp"
#include "utils.hpp"

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

	if (openAbout) about(&openAbout);

	return true;
}

bool MainWindow::controllers(int currentController, s_scePadSettings& scePadSettings, float scale) {
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

	ImGui::NewLine();
	ImGui::SetNextItemWidth(scale);
	ImGui::ColorPicker3(str("LightbarColor"), scePadSettings.led, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
	return true;
}

bool MainWindow::udp(int& currentController, float scale) {
	ImGui::SeparatorText(str("DSX Mods/UDP"));
	ImGui::Text(std::string(strr("Status") + ":").c_str()); ImGui::SameLine();

	if (m_udp.isActive())
		ImGui::TextColored(ImVec4(0, 1, 0, 1), m_strings.getString("Active").c_str());
	else
		ImGui::TextColored(ImVec4(1, 0, 0, 1), m_strings.getString("Inactive").c_str());

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

	if (ImGui::Checkbox(str("Audio passthrough"), &scePadSettings.audioPassthrough)) {
		if (scePadSettings.audioPassthrough) {
			if (!m_audio.startByUserId(currentController + 1)) {
				LOGE("Failed to start audio passthrough");
				scePadSettings.audioPassthrough = false;
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
	#if (!defined(__linux__)) && (!defined(__MACOS__))
		ImGui::TextColored(ImVec4(1, 0, 0, 1), str("Failed to start"));
	#else
		ImGui::TextColored(ImVec4(1, 0, 0, 1), str("Audio passthrough is not available on this platform"));
	#endif
	}
	else if (!failedToStart && scePadSettings.audioPassthrough) {
		ImGui::SetNextItemWidth(400);
		ImGui::SliderFloat(str("Haptics intensity"), &scePadSettings.hapticIntensity, 0.0f, 5.0f);
	}

	ImGui::Text(str("AudioOutputPath"));
	ImGui::RadioButton(str("StereoHeadset"), &scePadSettings.audioPath, SCE_PAD_AUDIO_PATH_STEREO_HEADSET);
	ImGui::RadioButton(str("MonoLeftHeadset"), &scePadSettings.audioPath, SCE_PAD_AUDIO_PATH_MONO_LEFT_HEADSET);
	ImGui::RadioButton(str("MonoLeftHeadsetAndSpeaker"), &scePadSettings.audioPath, SCE_PAD_AUDIO_PATH_MONO_LEFT_HEADSET_AND_SPEAKER);
	ImGui::RadioButton(str("OnlySpeaker"), &scePadSettings.audioPath, SCE_PAD_AUDIO_PATH_ONLY_SPEAKER);

	ImGui::SetNextItemWidth(400);
	ImGui::SliderInt(str("SpeakerVolume"), &scePadSettings.speakerVolume, 0, 8, "%d");
	ImGui::SetNextItemWidth(400);
	ImGui::SliderInt(str("MicrophoneGain"), &scePadSettings.micGain, 0, 8, "%d");
	return true;
}

bool MainWindow::adaptiveTriggers(s_scePadSettings& scePadSettings) {
	if (m_udp.isActive())
		return false;

	ImGui::SeparatorText(str("AdaptiveTriggers"));

	ImGui::Text(str("SelectedTrigger"));
	ImGui::RadioButton("L2", &scePadSettings.uiSelectedTrigger, L2); ImGui::SameLine();
	ImGui::RadioButton("R2", &scePadSettings.uiSelectedTrigger, R2);

	ImGui::Text(str("TriggerFormat"));
	ImGui::RadioButton("Sony", &scePadSettings.uiTriggerFormat[scePadSettings.uiSelectedTrigger], SONY_FORMAT); ImGui::SameLine();
	ImGui::RadioButton("DSX", &scePadSettings.uiTriggerFormat[scePadSettings.uiSelectedTrigger], DSX_FORMAT);

	int currentlySelectedTrigger = scePadSettings.uiSelectedTrigger;
	int currentTriggerFormat = scePadSettings.uiTriggerFormat[currentlySelectedTrigger];

	static int currentSonyItem[TRIGGER_COUNT] = {0,0};
	static int currentDSXItem[TRIGGER_COUNT] = {0,0};
	static std::vector<std::string> sonyItems = { TriggerStringSony::OFF, TriggerStringSony::FEEDBACK, TriggerStringSony::WEAPON, TriggerStringSony::VIBRATION, TriggerStringSony::SLOPE_FEEDBACK, TriggerStringSony::MULTIPLE_POSITION_FEEDBACK, TriggerStringSony::MULTIPLE_POSITION_VIBRATION };
	static std::vector<std::string> dsxItems = { TriggerStringDSX::Normal, TriggerStringDSX::GameCube, TriggerStringDSX::VerySoft, TriggerStringDSX::Soft, TriggerStringDSX::Medium, TriggerStringDSX::Hard, TriggerStringDSX::VeryHard , TriggerStringDSX::Hardest, TriggerStringDSX::VibrateTrigger, TriggerStringDSX::VibrateTriggerPulse, TriggerStringDSX::Choppy, TriggerStringDSX::CustomTriggerValue, TriggerStringDSX::Resistance,TriggerStringDSX::Bow,TriggerStringDSX::Galloping,TriggerStringDSX::SemiAutomaticGun, TriggerStringDSX::AutomaticGun, TriggerStringDSX::Machine, TriggerStringDSX::VIBRATE_TRIGGER_10Hz};
	
	ImGui::SetNextItemWidth(450);
	if (ImGui::BeginCombo(str("TriggerMode"), currentTriggerFormat == SONY_FORMAT ? scePadSettings.uiSelectedSonyTriggerMode[currentlySelectedTrigger].c_str()
		: scePadSettings.uiSelectedDSXTriggerMode[currentlySelectedTrigger].c_str())) {
		std::vector<std::string>& items = (currentTriggerFormat == SONY_FORMAT) ? sonyItems : dsxItems;
		int& currentItem = (currentTriggerFormat == SONY_FORMAT) ? currentSonyItem[currentlySelectedTrigger] : currentDSXItem[currentlySelectedTrigger];

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
		scePadSettings.uiSelectedSonyTriggerMode[currentlySelectedTrigger] = sonyItems[currentSonyItem[currentlySelectedTrigger]];
	}
	else {
		scePadSettings.isLeftUsingDsxTrigger = currentlySelectedTrigger == L2 ? true : scePadSettings.isLeftUsingDsxTrigger;
		scePadSettings.isRightUsingDsxTrigger = currentlySelectedTrigger == R2 ? true : scePadSettings.isRightUsingDsxTrigger;
		scePadSettings.uiSelectedDSXTriggerMode[currentlySelectedTrigger] = dsxItems[currentDSXItem[currentlySelectedTrigger]];
	}

	if(scePadSettings.uiTriggerFormat[currentlySelectedTrigger] == SONY_FORMAT)
	{
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
			ImGui::SliderInt(str("EndPosition"), &endPosition, scePadSettings.uiParameters[currentlySelectedTrigger][0]+1, 8); ImGui::SetNextItemWidth(450);
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
			ImGui::SliderInt(str("StartPosition"), &startPosition, 1, endPosition-1); ImGui::SetNextItemWidth(450);
			ImGui::SliderInt(str("EndPosition"), &endPosition, startPosition+1, 9); ImGui::SetNextItemWidth(450);
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
			static const std::vector<std::string> customTriggerList = { "Off", "Rigid", "Rigid_A", "Rigid_B", "Rigid_AB", "Pulse", "Pulse_A", "Pulse_B", "Pulse_AB"};
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
			ImGui::SliderInt(str("End"), &end, start+1, 8); ImGui::SetNextItemWidth(450);
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
			ImGui::SliderInt(str("Start"), &start, 0, end-1); ImGui::SetNextItemWidth(450);
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
			if (end < start) end = start+1;
			if (force > 8) force = 8;
			if (force < 1) force = 1;

			ImGui::SetNextItemWidth(450);
			ImGui::SliderInt(str("Start"), &start, 2, end-1); ImGui::SetNextItemWidth(450);
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

	for (int i = 0; i < TRIGGER_COUNT; i++) {
		std::vector<uint8_t> vec;

		for (int j = 0; j < MAX_PARAM_COUNT; j++) {
			vec.push_back(scePadSettings.uiParameters[i][j]);
		}

		if (currentTriggerFormat == SONY_FORMAT) {
			if (auto it = sonyTriggerHandlers.find(sonyItems[currentSonyItem[i]]); it != sonyTriggerHandlers.end())
				it->second(scePadSettings, i, vec);
		}
		else {
			if (auto it = dsxTriggerHandlers.find(dsxItems[currentDSXItem[i]]); it != dsxTriggerHandlers.end()) 
				it->second(scePadSettings, i, vec);		
		}
	}

	return true;
}

bool MainWindow::keyboardAndMouseMapping(s_scePadSettings& scePadSettings) {
	ImGui::SeparatorText(str("KeyboardAndMouseMapping"));
	ImGui::Checkbox("Analog WSAD emulation", &scePadSettings.emulateAnalogWsad);
	return true;
}

bool MainWindow::analogSticks(s_scePadSettings& scePadSettings, s_ScePadData& state) {
	ImGui::SeparatorText(str("AnalogSticks"));

	const int previewSize = 100;
	const float radius = static_cast<float>(previewSize);
	const ImU32 whiteColor = IM_COL32(255, 255, 255, 255);
	const ImU32 redColor = IM_COL32(255, 0, 0, 255);
	const ImU32 greenColor = IM_COL32(0, 255, 0, 255);

	auto drawStick = [&](const s_SceStickData& stick, bool isPressed, int deadzone, ImVec2 centerPos) {
		ImGui::GetWindowDrawList()->AddCircle(centerPos, radius, isPressed ? redColor : whiteColor, 32, 2.0f);
		float normDeadzone = (deadzone * radius) / 255;
		ImGui::GetWindowDrawList()->AddCircle(centerPos, normDeadzone, greenColor, 32, 2.0f);

		float normX = (stick.X - 128) / 127.0f;
		float normY = -((stick.Y - 128) / 127.0f);

		ImVec2 stickPos = centerPos;
		stickPos.x += normX * radius;
		stickPos.y -= normY * radius; 

		ImGui::GetWindowDrawList()->AddCircleFilled(stickPos, 5, redColor, 32);
		ImGui::GetWindowDrawList()->AddText(stickPos, whiteColor, std::to_string(stick.X).c_str());
		ImGui::GetWindowDrawList()->AddText(ImVec2(stickPos.x - 19, stickPos.y - 30), whiteColor, std::to_string(stick.Y).c_str());
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
	ImGui::SliderInt(str("L2Deadzone"), &scePadSettings.leftStickDeadzone, 0, 255);
	ImGui::SetNextItemWidth(400);
	ImGui::SliderInt(str("R2Deadzone"), &scePadSettings.rightStickDeadzone, 0, 255);

	return true;
}

bool MainWindow::emulation(int currentController, s_scePadSettings& scePadSettings) {
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
		ImGui::Text("Real controller settings");

		if (m_isAdminWindows) {
			if (ImGui::Button("Hide")) {
				hideController(scePadGetPath(g_scePad[currentController]));
			}
			ImGui::SameLine();
			if (ImGui::Button("Unhide")) {
				unhideController(scePadGetPath(g_scePad[currentController]));
			}
		}
		else {
			ImGui::TextColored(ImVec4(1, 1, 0, 1), str("EmuFeaturesNonAdmin"));
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

	menuBar();
	if (controllers(c, scePadSettings[c], scale)) {
		udp(c, scale);
		emulation(c, scePadSettings[c]);
		led(scePadSettings[c], scale);
		adaptiveTriggers(scePadSettings[c]);
		audio(c, scePadSettings[c]);
		keyboardAndMouseMapping(scePadSettings[c]);
		analogSticks(scePadSettings[c], state);
	}

	m_selectedController = c;

	ImGui::End();
}

int MainWindow::getSelectedController() {
	return m_selectedController;
}
