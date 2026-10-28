#include "application.hpp"

#ifdef WINDOWS
#include <Windows.h>
#include <shellapi.h>
#include <iostream>
#include <DbgHelp.h>
#include <platform_folders.h>
#include <filesystem>

#pragma comment(lib, "Dbghelp.lib")

// Resolved at startup: the install folder (Program Files) is not writable for a normal user,
// so a dump written next to the exe is silently lost.
static wchar_t g_DumpDirectory[MAX_PATH] = L"";

static void PrepareDumpDirectory()
{
	try {
		std::filesystem::path directory = std::filesystem::u8path(sago::getDocumentsFolder() + "/DSY/CrashDumps/");
		std::error_code ec;
		std::filesystem::create_directories(directory, ec);
		std::wstring directoryString = directory.make_preferred().wstring();
		// Leave room for "crash_YYYY-MM-DD_HH-MM-SS.dmp"; otherwise keep the old relative name.
		if (!ec && directoryString.size() + 32 < MAX_PATH) {
			wcscpy_s(g_DumpDirectory, directoryString.c_str());
		}
	}
	catch (...) {
	}
}

static std::wstring GetDumpFileName()
{
	SYSTEMTIME st;
	GetLocalTime(&st);

	wchar_t filename[MAX_PATH];

	swprintf_s(
		filename,
		L"%lscrash_%04d-%02d-%02d_%02d-%02d-%02d.dmp",
		g_DumpDirectory,
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

	PrepareDumpDirectory();
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
