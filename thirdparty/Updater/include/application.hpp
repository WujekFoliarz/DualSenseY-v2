#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <string>

class Application {
public:
	Application();
	~Application();

	enum class Platform {
		Windows,
		Linux,
		Android,
		Unknown
	};

	static Platform GetPlatform() {
	#if defined(WINDOWS)
		return Platform::Windows;
	#elif defined(LINUX)
		return Platform::Linux;
	#elif defined(ANDROID)
		return Platform::Android;
	#else
		return Platform::Unknown;
	#endif
	}

	static Application* GetApplication();

	bool Run(const std::string& Url, const std::string& FileName);
};


#endif // APPLICATION_HPP