#include "strings.hpp"
#include <fstream>
#include <filesystem>

std::string CountryCodeToFile(const std::string& code) {
	return std::string(RESOURCES_PATH "translations/" + code + ".json");
}

void Strings::ReadStringsFromJson(const std::string& path) {
	using json = nlohmann::json;

	if (!std::filesystem::exists(std::filesystem::path(CountryCodeToFile("en"))))
		return;

	std::ifstream fEn(CountryCodeToFile("en"));
	json dataOrg = json::parse(fEn);

	for (auto& [key, value] : dataOrg.items()) {
		m_Strings[key] = value;
	}

	if (std::filesystem::exists(std::filesystem::path(path))) {
		std::ifstream f(path);
		json data = json::parse(f);

		for (auto& [key, value] : data.items()) {
			m_Strings[key] = value;
		}
	}
}

std::string Strings::GetString(const std::string& key) {
	if (m_Strings.find(key) != m_Strings.end()) {
		return m_Strings[key];
	}

	return "<" + key + ">";
}

