#ifndef STRINGS_H
#define STRINGS_H

#include <unordered_map>
#include <nlohmann/json.hpp>
#include <string>

std::string countryCodeToFile(const std::string& code);

class Strings {
private:
	std::unordered_map<std::string, std::string> m_strings;
public:
	void readStringsFromJson(const std::string& path);
	std::string getString(const std::string& key);
};

#endif