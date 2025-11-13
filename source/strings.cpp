#include "strings.hpp"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <codecvt>
#include <locale>

std::string CountryCodeToFile(const std::string& code) {
	return std::string(RESOURCES_PATH "translations/" + code + ".json");
}

std::string ReverseUTF8(const std::string& s) {
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
	auto u32 = conv.from_bytes(s);
	std::reverse(u32.begin(), u32.end());
	return conv.to_bytes(u32);
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

	std::filesystem::path pathToTranslatedJson(path);
	if (std::filesystem::exists(pathToTranslatedJson)) {
		std::ifstream f(pathToTranslatedJson);
		json data = json::parse(f);

		for (auto& [key, value] : data.items()) {
			if(pathToTranslatedJson.stem().string() == "ar")
				m_Strings[key] = ReverseUTF8(value);
			else	
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

