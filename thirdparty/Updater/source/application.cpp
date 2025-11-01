#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "application.hpp"
#include <iostream>
#include "log.hpp"
#include <zip.h>
#include <process.hpp>
#include <httplib.h>
#include <fstream>
#include <filesystem>

inline Application* s_Application = nullptr;

Application* Application::GetApplication() {
	return s_Application;
}

Application::Application() {
	s_Application = this;
}

Application::~Application() {
}

struct UrlParts {
    std::string scheme;
    std::string domain;
    std::string path;
};

UrlParts ParseUrl(const std::string& url) {
    UrlParts parts;

    // Find scheme
    size_t schemeEnd = url.find("://");
    if (schemeEnd != std::string::npos) {
        parts.scheme = url.substr(0, schemeEnd);
    }
    else {
        schemeEnd = -3; // so adding 3 starts after 0
        parts.scheme = "http";
    }

    // Find domain start and path start
    size_t domainStart = schemeEnd + 3;
    size_t pathStart = url.find('/', domainStart);
    if (pathStart != std::string::npos) {
        parts.domain = url.substr(domainStart, pathStart - domainStart);
        parts.path = url.substr(pathStart); // include '/'
    }
    else {
        parts.domain = url.substr(domainStart);
        parts.path = "/";
    }

    return parts;
}

bool DownloadFile(const std::string& Url, const std::string& UrlPath, const std::string& OutPath) {
    httplib::Client cli(Url);
    cli.set_follow_location(true);
    auto res = cli.Get(UrlPath);
    if (res && res->status == 200) {
        std::ofstream file(OutPath, std::ios::binary);
        file.write(res->body.c_str(), res->body.size());
        return true;
    }
    std::cout << "Failed to download! " << (int)res->status << std::endl;
    return false;
}

bool Application::Run(const std::string& Url, const std::string& FileName) {
    //std::cout << "Arguments: " << Url << " " << FileName << std::endl;
    std::cout << "Please wait...." << std::endl;

    UrlParts p = ParseUrl(Url);
    DownloadFile(p.domain, p.path, FileName);

    zip_extract(FileName.c_str(), "./", nullptr, nullptr);

    std::filesystem::path filePath("DualSenseY.exe");
    if (std::filesystem::exists(filePath)) {
        TinyProcessLib::Process process("DualSenseY.exe");
    }

	return true;
}
