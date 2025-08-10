#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <memory>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "scePadSettings.hpp"

struct glfwDeleter {
	void operator()(GLFWwindow* window) {
		glfwDestroyWindow(window);
	}
};

class Application {
private:
	std::unique_ptr<GLFWwindow, glfwDeleter> m_glfwWindow;
	s_scePadSettings m_scePadSettings[4] = {};
	bool isMinimized();
	void disableControllerInputIfMinimized();
public:
	enum class Platform {
		Windows,
		Linux,
		Android,
		Unknown
	};

	inline Platform getPlatform() {
	#if defined(__linux__)
		return Platform::Linux;
	#elif defined(__ANDROID__)
		return Platform::Android;
	#elif defined(_WIN32)
		return Platform::Windows;
	#else
		return Platform::Unknown;
	#endif
	}
	
	bool run();
	void createWindow();
	void setStyleAndColors();
	Application() = default;
	~Application();
};


#endif // APPLICATION_HPP