
#include "application.hpp"
#include "log.hpp"

#if defined(_WIN32) && (!defined(__linux__) && !defined(__APPLE__))
#include <Windows.h>
#include <consoleapi.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <win32WindowStuff.hpp>
#include <winuser.h>
#endif

#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>
#include <imgui.h>
#include <duaLib.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_glfw.h>
#include <stb_image/stb_image.h>
#include <tray.hpp>

#include "mainWindow.hpp"
#include "strings.hpp"
#include "audioPassthrough.hpp"
#include "udp.hpp"
#include "controllerEmulation.hpp"
#include "scePadHandle.hpp"
#include "keyboardMouseMapper.hpp"
#include "client.hpp"

#if !defined(__linux__) && !defined(__MACOS__)
bool colorsChanged = false;
WNDPROC originalWndProc = nullptr;
LRESULT CALLBACK CustomWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		case WM_DWMCOLORIZATIONCOLORCHANGED:
		{
			colorsChanged = true;
			return 0;
		}
		default:
			return CallWindowProc(originalWndProc, hwnd, msg, wParam, lParam);
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}
#endif

bool Application::IsMinimized() {
	ImGuiIO& io = ImGui::GetIO();
	bool isMinimized = glfwGetWindowAttrib(m_GlfwWindow.get(), GLFW_ICONIFIED);
	bool isFocused = glfwGetWindowAttrib(m_GlfwWindow.get(), GLFW_FOCUSED);
	return (isMinimized || !isFocused);
}

void Application::DisableControllerInputIfMinimized() {
	ImGuiIO& io = ImGui::GetIO();

	if (IsMinimized()) {
		io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
	}
	else {
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  
	}
}

bool Application::Run() {
	// Delete update.zip if present
	remove("update.zip");

	Platform platform = Application::GetPlatform();
	#pragma region Initialize duaLib
	s_ScePadInitParam initParam = {};
	initParam.allowBT = true;
	scePadInit3(&initParam);
	g_ScePad[0] = scePadOpen(1, 0, 0);
	g_ScePad[1] = scePadOpen(2, 0, 0);
	g_ScePad[2] = scePadOpen(3, 0, 0);
	g_ScePad[3] = scePadOpen(4, 0, 0);
	scePadSetParticularMode(true);
#pragma endregion
	#if (!defined(PRODUCTION_BUILD) || PRODUCTION_BUILD == 0) && defined(_WIN32) && (!defined(__linux__) && !defined(__APPLE__))
	AllocConsole();
	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONO UT$", "w", stderr);
	freopen_s(&fp, "CONIN$", "r", stdin);
	#endif


	InitializeWindow();
	ImGuiIO& io = ImGui::GetIO();
	AudioPassthrough audio = {};
	UDP udp = {};
	Vigem vigem(m_ScePadSettings, udp);
	Strings strings = {};
	KeyboardMouseMapper keyboardMouseMapper(m_ScePadSettings);
	Client client(m_ScePadSettings);

	LoadAppSettings(&m_AppSettings);
	io.FontDefault = io.Fonts->Fonts[g_FontIndex[m_AppSettings.SelectedLanguage]];

	client.Start();
	if(!m_AppSettings.DontConnectToServerOnStart) client.Connect();
	client.AllowedToHostController = vigem.IsVigemConnected();
	vigem.SetPeerControllerDataPointer(client.GetActivePeerControllerMap());

	// Windows
	MainWindow main(strings, audio, vigem, udp, m_AppSettings, client);

	strings.ReadStringsFromJson(CountryCodeToFile(m_AppSettings.SelectedLanguage));

	bool active = false;
	uint32_t occasionalFrameWhenMinimized = 0;
	

#ifdef WINDOWS
	HANDLE hTimer = CreateWaitableTimer(NULL, TRUE, NULL);
	LARGE_INTEGER liDueTime;
	liDueTime.QuadPart = -100000LL;
#endif

	int display_w, display_h = 0;
	float xscale, yscale = 1;
	while (!glfwWindowShouldClose(m_GlfwWindow.get())) {
		bool v_isMinimized = IsMinimized();
		occasionalFrameWhenMinimized = v_isMinimized ? occasionalFrameWhenMinimized + 1 : 0;

		#pragma region ImGUI
		bool finishFrame = false;
		if (v_isMinimized && occasionalFrameWhenMinimized > 500 || !v_isMinimized) {
			glClear(GL_COLOR_BUFFER_BIT);
			glClearColor(0, 0, 0, 0);
			glfwGetFramebufferSize(m_GlfwWindow.get(), &display_w, &display_h);
			glViewport(0, 0, display_w, display_h);
			glfwGetWindowContentScale(m_GlfwWindow.get(), &xscale, &yscale);

		#if !defined(__linux__) && !defined(__MACOS__)
			if (colorsChanged) {
				SetStyleAndColors();
				colorsChanged = false;
			}
		#endif

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			io.FontGlobalScale = xscale + 0.5;
			finishFrame = true;
		}
		#pragma endregion

		int selectedController = main.GetSelectedController();
		vigem.SetSelectedController(selectedController);
		client.SetSelectedController(selectedController);
		udp.SetVibrationToUdpConfig(m_ScePadSettings[selectedController].rumbleFromEmulatedController);
		audio.Validate();	
		
		for (int i = 0; i < 4; i++) {
			LoadDefaultConfigs(i, &m_ScePadSettings[i]);
			applySettings(i, i == (selectedController) && udp.IsActive() ? udp.GetSettings() : m_ScePadSettings[i], audio);
		}

		#pragma region ImGUI + GLFW
		DisableControllerInputIfMinimized();
		glfwPollEvents();

		if (finishFrame) {
			main.Show(m_ScePadSettings, xscale);
			ImGui::Render();
			ImDrawData* drawData = ImGui::GetDrawData();
			ImGui_ImplOpenGL3_RenderDrawData(drawData);
			glfwSwapBuffers(m_GlfwWindow.get());
			occasionalFrameWhenMinimized = 0;
			ImGui::EndFrame();
		}

		#pragma endregion	

	#ifdef WINDOWS
		SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0);
		WaitForSingleObject(hTimer, INFINITE);
	#else
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	#endif
	}

#ifdef WINDOWS
	CloseHandle(hTimer);
#endif
	return true;
}

void Application::InitializeWindow() {
	glfwInit();
	m_GlfwWindow = std::unique_ptr<GLFWwindow, glfwDeleter>(glfwCreateWindow(1000, 720, "DualSenseY", nullptr, nullptr));
	glfwMakeContextCurrent(m_GlfwWindow.get());
	glfwSwapInterval(1);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		LOGE("GLAD couldn't be loaded");
	}

	glfwSetWindowCloseCallback(m_GlfwWindow.get(), [](GLFWwindow* window) {
		glfwSetWindowShouldClose(window, true);
	});
	
	ImGui::CreateContext();

	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#if defined(_WIN32) && (!defined(__linux__) && !defined(__APPLE__))
	HWND hwnd = glfwGetWin32Window(m_GlfwWindow.get());
	EnableBlurBehind(hwnd);
	SetDarkTitleBar(hwnd, true);
	originalWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)CustomWndProc);
#endif

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(m_GlfwWindow.get(), true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
	ImGui_ImplOpenGL3_Init();

	SetStyleAndColors();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

	ImVector<ImWchar> ranges;
	ImFontGlyphRangesBuilder builder;
	builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
	builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
	builder.AddRanges(io.Fonts->GetGlyphRangesKorean());
	builder.AddRanges(io.Fonts->GetGlyphRangesJapanese());
	builder.BuildRanges(&ranges);

	// Font index in appSettings.hpp
	ImFont* regular = io.Fonts->AddFontFromFileTTF(RESOURCES_PATH "fonts/Saira_Expanded-MediumItalic.ttf", 20, nullptr, ranges.Data);
	ImFont* japaneseAndCyrillic = io.Fonts->AddFontFromFileTTF(RESOURCES_PATH "fonts/Murecho-Regular.ttf", 20, nullptr, ranges.Data);
	ImFont* korean = io.Fonts->AddFontFromFileTTF(RESOURCES_PATH "fonts/AstaSans-Light.ttf", 20, nullptr, ranges.Data);
	ImFont* thai = io.Fonts->AddFontFromFileTTF(RESOURCES_PATH "fonts/Kanit-LightItalic.ttf", 20, nullptr, ranges.Data);
	ImFont* chineseSimplified = io.Fonts->AddFontFromFileTTF(RESOURCES_PATH "fonts/NotoSansSC-Regular.ttf", 20, nullptr, ranges.Data);
	
	GLFWimage image;
	image.pixels = stbi_load(RESOURCES_PATH "images/iconWhite.png", &image.width, &image.height, 0, 4); // RGBA
	if (image.pixels) {
		glfwSetWindowIcon(m_GlfwWindow.get(), 1, &image);
		stbi_image_free(image.pixels);
	}

	assert(m_GlfwWindow.get() != nullptr);
	LOGI("Window created");
}

void Application::SetStyleAndColors() {
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 10.0f;
	style.ChildRounding = 10.0f;
	style.PopupRounding = 10.0f;
	style.FrameRounding = 10.0f;
	style.GrabRounding = 10.0f;
	style.TabRounding = 10.0f;
	style.ScrollbarSize = 20.0f;

	ImVec4* colors = style.Colors;
	ImVec4 baseColor = ImVec4(0.369f, 0.075f, 0.929f, 1.0f);

#ifdef WINDOWS
	DWORD color = 0;
	BOOL opaque = FALSE;

	HRESULT hr = DwmGetColorizationColor(&color, &opaque);
	if (SUCCEEDED(hr)) {
		// color is in ARGB format (alpha in highest byte)
		BYTE a = (color >> 24) & 0xFF;
		BYTE r = (color >> 16) & 0xFF;
		BYTE g = (color >> 8) & 0xFF;
		BYTE b = color & 0xFF;

		std::cout << "Accent color (ARGB): " << std::dec << "A=" << (int)a << " " << "R=" << (int)r << " " << "G=" << (int)g << " " << "B=" << (int)b << std::endl;

		baseColor.x = r / 255.0f;
		baseColor.y = g / 255.0f;
		baseColor.z = b / 255.0f;
		baseColor.w = a / 255.0f;
	}
	else {
		std::cout << "Failed to get colorization color. HRESULT: " << hr << std::endl;
	}
#endif

	colors[ImGuiCol_WindowBg] = ImVec4(0, 0, 0, 0.0f);
	colors[ImGuiCol_Border] = ImVec4(0, 0, 0, 0.0f);
	colors[ImGuiCol_Button] = ImVec4(0.47, 0.38, 1, 1.0f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.60f, 0.90f, 1.0f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.45f, 0.75f, 1.0f);
	colors[ImGuiCol_FrameBg] = ImVec4(baseColor);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(ImVec4(baseColor.x - 0.069f, baseColor.y - 0.169f, baseColor.z - 0.069f, baseColor.w));
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.10f, 0.50f, 0.80f, 1.0f);
	colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	colors[ImGuiCol_SliderGrab] = ImVec4(baseColor.x - 0.569f, baseColor.y - 0.369f, baseColor.z - 0.569f, baseColor.w);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(ImVec4(baseColor.x - 0.069f, baseColor.y - 0.169f, baseColor.z - 0.069f, baseColor.w));
	colors[ImGuiCol_CheckMark] = ImVec4(baseColor.x - 0.569f, baseColor.y - 0.369f, baseColor.z - 0.569f, baseColor.w);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.05f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.0f, 0.0f, 0.2f, 0.3f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.30f, 0.60f, 0.90f, 0.5f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.47, 0.38, 1, 0.5f);
}

void Application::SetupTray() {
	Tray::Tray tray("DualSenseY", RESOURCES_PATH "images/icon.ico");
	tray.addEntry(Tray::Button("Exit", [&] {
		tray.exit();
	}));
}

Application::~Application() {
	// Unhide controllers
#ifdef WINDOWS
	if (IsRunningAsAdministratorWindows()) {
		for(int i = 0;i<4;i++)
			UnhideController(scePadGetPath(g_ScePad[i]));
	}

	if (m_AppSettings.DisableAllBluetoothControllersOnExit) {
		for (int i = 0; i < 4; i++)
			DisableBluetoothDevice(scePadGetMacAddress(g_ScePad[i]));
	}
#endif
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	scePadTerminate();
}