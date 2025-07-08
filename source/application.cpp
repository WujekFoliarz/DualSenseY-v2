#include "application.hpp"
#include <iostream>
#include "log.hpp"

#if defined(_WIN32) && (!defined(__linux__) && !defined(__APPLE__))
#include <Windows.h>
#include <consoleapi.h>
#endif

#include <cassert>
#include <imgui.h>
#include <duaLib.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_glfw.h>

#include "mainWindow.hpp"
#include "strings.hpp"
#include "audioPassthrough.hpp"
#include <thread>
#include <chrono>
#include "controllerEmulation.hpp"

std::thread g_vigemThread;
std::atomic<bool> g_vigemThreadRunning = true;

bool Application::run() {
	Platform platform = Application::getPlatform();
	#pragma region Initialize duaLib
	s_ScePadInitParam initParam = {};
	initParam.allowBT = true;
	scePadInit3(&initParam);
	m_scePadSettings[0].handle = scePadOpen(1, 0, 0);
	m_scePadSettings[1].handle = scePadOpen(2, 0, 0);
	m_scePadSettings[2].handle = scePadOpen(3, 0, 0);
	m_scePadSettings[3].handle = scePadOpen(4, 0, 0);
#pragma endregion
	#if (!defined(PRODUCTION_BUILD) || PRODUCTION_BUILD == 0) && defined(_WIN32) && (!defined(__linux__) && !defined(__APPLE__))
	AllocConsole();
	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONO UT$", "w", stderr);
	freopen_s(&fp, "CONIN$", "r", stdin);
	#endif

	createWindow();
	AudioPassthrough audio = {};
	Vigem vigem = {};
	Strings strings; // translations
	
	if (vigem.isVigemConnected()) {
		g_vigemThread = std::thread(emulatedControllerUpdate, std::ref(vigem), std::ref(m_scePadSettings), std::ref(g_vigemThreadRunning));
		g_vigemThread.detach();
	}

	// Windows
	MainWindow main(strings, audio, vigem);

	strings.readStringsFromJson(countryCodeToFile("en"));

	bool active = false;

	while (!glfwWindowShouldClose(m_glfwWindow.get())) {

		#pragma region ImGUI
		int display_w, display_h;
		float xscale, yscale;
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0, 0, 0, 1);
		glfwGetFramebufferSize(m_glfwWindow.get(), &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glfwGetWindowContentScale(m_glfwWindow.get(), &xscale, &yscale);
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGuiIO& io = ImGui::GetIO();
		io.FontGlobalScale = xscale + 0.5;
		#pragma endregion

		audio.validate();
		main.show(m_scePadSettings, xscale);

		//ImGui::ShowDemoWindow();

		for (int i = 0; i < 4; i++) {
			applySettings(i+1, m_scePadSettings[i], audio);
		}

		#pragma region ImGUI + GLFW
		glfwPollEvents();
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(m_glfwWindow.get());	
		#pragma endregion

		std::this_thread::sleep_for(std::chrono::milliseconds(16)); // around 60ish frames
	}

	return true;
}

void Application::createWindow() {
	glfwInit();
	m_glfwWindow = std::unique_ptr<GLFWwindow, glfwDeleter>(glfwCreateWindow(1280, 720, "DualSenseY", nullptr, nullptr));
	glfwMakeContextCurrent(m_glfwWindow.get());
	glfwSwapInterval(0);

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
	g_vigemThreadRunning = false;
	if (g_vigemThread.joinable()) {
		g_vigemThread.join();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	scePadTerminate();
}
