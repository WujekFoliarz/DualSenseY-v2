#include "strings.hpp"
#include <fstream>

std::string countryCodeToFile(const std::string& code) {
	return std::string(RESOURCES_PATH "translations/" + code + ".json");
}

void Strings::readStringsFromJson(const std::string& path) {
	using json = nlohmann::json;

	std::ifstream f(path);
	json data = json::parse(f);

	std::ifstream f2(countryCodeToFile("en"));
	json dataOrg = json::parse(f2);
	
	for (auto& [key, value] : dataOrg.items()) {
		m_strings[key] = value;
	}

	for (auto& [key, value] : data.items()) {
		m_strings[key] = value;
	}
}

std::string Strings::getString(const std::string& key) {
	if (m_strings.find(key) != m_strings.end()) {
		return m_strings[key];
	}

	return "<" + key + ">";
}

