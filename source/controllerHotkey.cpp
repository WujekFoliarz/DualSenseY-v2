#include "controllerHotkey.hpp"
#include <duaLib.h>

bool IsHotkeyActive(uint32_t RequiredButtons, uint32_t ActiveButtons) {
	return (ActiveButtons & RequiredButtons) == RequiredButtons;
}

std::vector<std::string> GetActiveButtonNames(uint32_t ActiveButtons) {
    static const std::pair<uint32_t, const char*> ButtonNames[] = {
    {SCE_BM_CROSS, "Cross"},
    {SCE_BM_CIRCLE, "Circle"},
    {SCE_BM_SQUARE, "Square"},
    {SCE_BM_TRIANGLE, "Triangle"},
    {SCE_BM_L1, "L1"},
    {SCE_BM_R1, "R1"},
    {SCE_BM_L2, "L2"},
    {SCE_BM_R2, "R2"},
    {SCE_BM_SHARE, "Share"},
    {SCE_BM_OPTIONS, "Options"},
    {SCE_BM_L3, "L3"},
    {SCE_BM_R3, "R3"},
    {SCE_BM_PSBTN, "PS"},
    {SCE_BM_TOUCH, "Touchpad"},
    {SCE_BM_N_DPAD, "Dpad Up"},
    {SCE_BM_S_DPAD, "Dpad Down"},
    {SCE_BM_W_DPAD, "Dpad Left"},
    {SCE_BM_E_DPAD, "Dpad Right"},
    };

    std::vector<std::string> ActiveNames;
    ActiveNames.reserve(std::size(ButtonNames));

    for (auto& [mask, name] : ButtonNames) {
        if (ActiveButtons & mask)
            ActiveNames.emplace_back(name);
    }

    return ActiveNames;
}

std::string GetFormattedActiveButtonNames(uint32_t ActiveButtons) {
    std::string hotkeyFormatted = "";
    for (auto& btnName : GetActiveButtonNames(ActiveButtons)) {
        hotkeyFormatted += btnName + " + ";
    }
    if (hotkeyFormatted != "") {
        hotkeyFormatted.pop_back(); hotkeyFormatted.pop_back(); // Remove the last + and space
    }
    return hotkeyFormatted;
}
