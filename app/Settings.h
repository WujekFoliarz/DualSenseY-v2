#pragma once
#include <nlohmann/json.hpp>

class Settings
{
public:
    DualsenseUtils::InputFeatures ControllerInput;
    bool AudioToLED = false;
    bool DiscoMode = false;
    int lrumble = 0;
    int rrumble = 0;
    string lmodestr = "Off";
    string rmodestr = "Off";
    bool UseUDP = false;
    bool MicScreenshot = false;
    bool MicFunc = false;
    bool RumbleToAT = false;
    bool BatteryLightbar = false;
    EmuStatus emuStatus = None;

    nlohmann::json to_json() const {
        nlohmann::json j;
        j["ControllerInput"] = ControllerInput.to_json();
        j["AudioToLED"] = AudioToLED;
        j["DiscoMode"] = DiscoMode;
        j["lrumble"] = lrumble;
        j["rrumble"] = rrumble;
        j["lmodestr"] = lmodestr;
        j["rmodestr"] = rmodestr;
        j["UseUDP"] = UseUDP;
        j["MicScreenshot"] = MicScreenshot;
        j["MicFunc"] = MicFunc;
        j["BatteryLightbar"] = BatteryLightbar;
        j["emuStatus"] = static_cast<int>(emuStatus); // Assuming EmuStatus is an enum
        return j;
    }

    static Settings from_json(const nlohmann::json& j) {
        Settings settings;

        // Parse ControllerInput first
        if (j.contains("ControllerInput")) settings.ControllerInput = DualsenseUtils::InputFeatures::from_json(j.at("ControllerInput"));

        if (j.contains("AudioToLED"))       j.at("AudioToLED").get_to(settings.AudioToLED);
        if (j.contains("DiscoMode"))        j.at("DiscoMode").get_to(settings.DiscoMode);
        if (j.contains("lrumble"))          j.at("lrumble").get_to(settings.lrumble);
        if (j.contains("rrumble"))          j.at("rrumble").get_to(settings.rrumble);
        if (j.contains("lmodestr"))         j.at("lmodestr").get_to(settings.lmodestr);
        if (j.contains("rmodestr"))         j.at("rmodestr").get_to(settings.rmodestr);
        if (j.contains("UseUDP"))           j.at("UseUDP").get_to(settings.UseUDP);
        if (j.contains("MicScreenshot"))    j.at("MicScreenshot").get_to(settings.MicScreenshot);
        if (j.contains("MicFunc"))          j.at("MicFunc").get_to(settings.MicFunc);
        if (j.contains("BatteryLightbar"))          j.at("BatteryLightbar").get_to(settings.BatteryLightbar);
        if (j.contains("emuStatus"))        j.at("emuStatus").get_to(settings.emuStatus);

        return settings;
    }
};
