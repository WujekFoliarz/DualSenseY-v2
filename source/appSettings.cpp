#include "appSettings.hpp"
#include <platform_folders.h>
#include <filesystem>
#include <fstream>

static inline std::filesystem::path directory = std::filesystem::path(sago::getDocumentsFolder() + "/DSY/");
static inline std::filesystem::path filePath = directory / "Config.json";

void SaveAppSettings(AppSettings* appSettings) {
	nlohmann::json j = *appSettings;

	if (!std::filesystem::is_directory(directory))
		std::filesystem::create_directories(directory);

	std::ofstream(filePath) << j.dump(4);
}

void LoadAppSettings(AppSettings* appSettings) {
	if (std::filesystem::exists(filePath)) {
		std::ifstream ifs(filePath);
		if (!ifs) return;
		nlohmann::json j;
		ifs >> j;
		*appSettings = j.get<AppSettings>();
	}
}