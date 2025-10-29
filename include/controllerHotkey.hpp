#pragma once

#include <cstdint>
#include <string>
#include <vector>

bool IsHotkeyActive(uint32_t RequiredButtons, uint32_t ActiveButtons);
std::vector<std::string> GetActiveButtonNames(uint32_t ActiveButtons);
// BUTTON1 + BUTTON2...
std::string GetFormattedActiveButtonNames(uint32_t ActiveButtons);