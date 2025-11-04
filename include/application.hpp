#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <memory>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "scePadSettings.hpp"
#include "appSettings.hpp"

constexpr auto WIN32_MSG_WINDOW_MUTEX = "DSYMSG";

struct glfwDeleter {
	void operator()(GLFWwindow* window) {
		glfwDestroyWindow(window);
	}
};

class Application {
private:
	std::unique_ptr<GLFWwindow, glfwDeleter> m_GlfwWindow;
	s_scePadSettings m_ScePadSettings[4] = {};
	bool IsMinimized();
	void DisableControllerInputIfMinimized();
	AppSettings m_AppSettings = {};
	static void IconifyCallback(GLFWwindow* window, int iconified);
public:
	enum class Platform {
		Windows,
		Linux,
		Android,
		Unknown
	};

	inline Platform GetPlatform() {
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
	
	bool Run();
	void InitializeWindow();
	void SetStyleAndColors();
	void SetupTray();
	void HideWindowToTray();
	void RestoreWindowFromTray();
	Application() = default;
	~Application();
};


#endif // APPLICATION_HPP