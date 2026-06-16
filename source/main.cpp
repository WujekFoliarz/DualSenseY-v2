#include "application.hpp"

#ifdef WINDOWS
#include <Windows.h>
#include <shellapi.h>
#include <iostream>
#include <DbgHelp.h>

#pragma comment(lib, "Dbghelp.lib")

static std::wstring GetDumpFileName()
{
	SYSTEMTIME st;
	GetLocalTime(&st);

	wchar_t filename[MAX_PATH];

	swprintf_s(
		filename,
		L"crash_%04d-%02d-%02d_%02d-%02d-%02d.dmp",
		st.wYear,
		st.wMonth,
		st.wDay,
		st.wHour,
		st.wMinute,
		st.wSecond);

	return filename;
}

static LONG WINAPI CrashHandler(EXCEPTION_POINTERS* exceptionInfo)
{
	HANDLE hFile = CreateFileW(
		GetDumpFileName().c_str(),
		GENERIC_WRITE,
		0,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		MINIDUMP_EXCEPTION_INFORMATION dumpInfo{};
		dumpInfo.ThreadId = GetCurrentThreadId();
		dumpInfo.ExceptionPointers = exceptionInfo;
		dumpInfo.ClientPointers = FALSE;
		MINIDUMP_TYPE dumpType =
			(MINIDUMP_TYPE)(
				MiniDumpWithDataSegs |
				MiniDumpWithHandleData |
				MiniDumpWithThreadInfo |
				MiniDumpWithIndirectlyReferencedMemory |
				MiniDumpScanMemory |
				MiniDumpWithModuleHeaders |       
				MiniDumpWithFullAuxiliaryState |   
				MiniDumpWithProcessThreadData |
				MiniDumpWithFullMemoryInfo |
				MiniDumpWithUnloadedModules
				);

		MiniDumpWriteDump(
			GetCurrentProcess(),
			GetCurrentProcessId(),
			hFile,
			dumpType,
			&dumpInfo,
			nullptr,
			nullptr);

		CloseHandle(hFile);
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {

	SetUnhandledExceptionFilter(CrashHandler);

	if (AttachConsole(ATTACH_PARENT_PROCESS))
	{
		FILE* fp;
		freopen_s(&fp, "CONOUT$", "w", stdout);
		freopen_s(&fp, "CONOUT$", "w", stderr);
		freopen_s(&fp, "CONIN$", "r", stdin);
	}

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
