#include "strings.hpp"
#include <fstream>
#include <filesystem>

std::string countryCodeToFile(const std::string& code) {
	return std::string(RESOURCES_PATH "translations/" + code + ".json");
}

void Strings::readStringsFromJson(const std::string& path) {
	using json = nlohmann::json;

	if (!std::filesystem::exists(std::filesystem::path(countryCodeToFile("en"))))
		return;

	std::ifstream fEn(countryCodeToFile("en"));
	json dataOrg = json::parse(fEn);

	for (auto& [key, value] : dataOrg.items()) {
		m_strings[key] = value;
	}

	if (std::filesystem::exists(std::filesystem::path(path))) {
		std::ifstream f(path);
		json data = json::parse(f);

		for (auto& [key, value] : data.items()) {
			m_strings[key] = value;
		}
	}
}

std::string Strings::getString(const std::string& key) {
	if (m_strings.find(key) != m_strings.end()) {
		return m_strings[key];
	}

	return "<" + key + ">";
}

