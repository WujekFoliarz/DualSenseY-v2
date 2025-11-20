#include "application.hpp"

#ifdef WINDOWS
#include <Windows.h>
#include <shellapi.h>
#include <iostream>
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {

#if (!defined(PRODUCTION_BUILD) || PRODUCTION_BUILD == 0)
	AllocConsole();
	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONOUT$", "w", stderr);
	freopen_s(&fp, "CONIN$", "r", stdin);
#endif

	wchar_t exePath[MAX_PATH];
	GetModuleFileNameW(nullptr, exePath, MAX_PATH);
	std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
	std::filesystem::current_path(exeDir);

	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	std::string argument1 = "";
	if (argc >= 2 && argv[1]) {
		int size = WideCharToMultiByte(CP_UTF8, 0, argv[1], -1, nullptr, 0, nullptr, nullptr);
		argument1.resize(size - 1);
		WideCharToMultiByte(CP_UTF8, 0, argv[1], -1, argument1.data(), size - 1, nullptr, nullptr);
	}

	LocalFree(argv);

	Application application;
	application.Run(argument1);
}
#else
int main(int argc, char* argv[]) {
	#ifdef LINUX
	gtk_disable_setlocale();
	gtk_init(&argc, &argv);
	#endif
	
	Application application;
	application.Run();
}
#endif
