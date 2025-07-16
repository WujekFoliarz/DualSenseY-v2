#include "application.hpp"
#include "log.hpp"

#if defined(_WIN32) && (!defined(__linux__) && !defined(__APPLE__))
#include <Windows.h>
#include <consoleapi.h>
#endif

#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>
#include <imgui.h>
#include <duaLib.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_glfw.h>

#include "mainWindow.hpp"
#include "strings.hpp"
#include "audioPassthrough.hpp"
#include "udp.hpp"
#include "controllerEmulation.hpp"
#include "scePadHandle.hpp"

bool Application::isMinimized() {
	ImGuiIO& io = ImGui::GetIO();
	bool isMinimized = glfwGetWindowAttrib(m_glfwWindow.get(), GLFW_ICONIFIED);
	bool isFocused = glfwGetWindowAttrib(m_glfwWindow.get(), GLFW_FOCUSED);
	return (isMinimized || !isFocused);
}

void Application::disableControllerInputIfMinimized() {
	ImGuiIO& io = ImGui::GetIO();

	if (isMinimized()) {
		io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
	}
	else {
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  
	}
}


bool Application::run() {
	Platform platform = Application::getPlatform();
	#pragma region Initialize duaLib
	s_ScePadInitParam initParam = {};
	initParam.allowBT = true;
	scePadInit3(&initParam);
	g_scePad[0] = scePadOpen(1, 0, 0);
	g_scePad[1] = scePadOpen(2, 0, 0);
	g_scePad[2] = scePadOpen(3, 0, 0);
	g_scePad[3] = scePadOpen(4, 0, 0);
	scePadSetParticularMode(true);
#pragma endregion
	//#if (!defined(PRODUCTION_BUILD) || PRODUCTION_BUILD == 0) && defined(_WIN32) && (!defined(__linux__) && !defined(__APPLE__))
	AllocConsole();
	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONO UT$", "w", stderr);
	freopen_s(&fp, "CONIN$", "r", stdin);
	//#endif

	createWindow();
	AudioPassthrough audio = {};
	UDP udp = {};
	Vigem vigem(m_scePadSettings, udp);
	Strings strings = {}; // translations

	// Windows
	MainWindow main(strings, audio, vigem, udp);

	strings.readStringsFromJson(countryCodeToFile("en"));

	bool active = false;
	uint32_t occasionalFrameWhenMinimized = 0;

	while (!glfwWindowShouldClose(m_glfwWindow.get())) {

		#pragma region ImGUI
		int display_w, display_h;
		float xscale, yscale;
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0, 0, 0, 1);
		glfwGetFramebufferSize(m_glfwWindow.get(), &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glfwGetWindowContentScale(m_glfwWindow.get(), &xscale, &yscale);

		bool v_isMinimized = isMinimized();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGuiIO& io = ImGui::GetIO();
		io.FontGlobalScale = xscale + 0.5;
		#pragma endregion

		vigem.setSelectedController(main.getSelectedController());
		audio.validate();
		main.show(m_scePadSettings, xscale);

		for (int i = 0; i < 4; i++) {
			applySettings(i, i == (main.getSelectedController()) && udp.isActive() ? udp.getSettings() : m_scePadSettings[i], audio);
		}

		#pragma region ImGUI + GLFW
		disableControllerInputIfMinimized();
		glfwPollEvents();

		occasionalFrameWhenMinimized = v_isMinimized ? occasionalFrameWhenMinimized + 1 : 0;
		if (v_isMinimized && occasionalFrameWhenMinimized > 500 || !v_isMinimized) {
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			glfwSwapBuffers(m_glfwWindow.get());
			occasionalFrameWhenMinimized = 0;
		}

		ImGui::EndFrame();
		#pragma endregion	

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	return true;
}

void Application::createWindow() {
	glfwInit();
	m_glfwWindow = std::unique_ptr<GLFWwindow, glfwDeleter>(glfwCreateWindow(1280, 720, "DualSenseY", nullptr, nullptr));
	glfwMakeContextCurrent(m_glfwWindow.get());
	glfwSwapInterval(1);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		LOGE("GLAD couldn't be loaded");
	}

	glfwSetWindowCloseCallback(m_glfwWindow.get(), [](GLFWwindow* window) {
		glfwSetWindowShouldClose(window, true);
	});
	
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(m_glfwWindow.get(), true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
	ImGui_ImplOpenGL3_Init();

	assert(m_glfwWindow.get() != nullptr);
	LOGI("Window created");
}

Application::~Application() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	scePadTerminate();
}
