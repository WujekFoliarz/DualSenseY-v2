#ifndef STRINGS_H
#define STRINGS_H

#include <unordered_map>
#include <nlohmann/json.hpp>
#include <string>

std::string CountryCodeToFile(const std::string& code);

class Strings {
private:
	std::unordered_map<std::string, std::string> m_Strings;
public:
	void ReadStringsFromJson(const std::string& path);
	std::string GetString(const std::string& key);
};

#endif