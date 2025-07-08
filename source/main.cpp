#include "application.hpp"

#if defined(__linux__) || defined(__APPLE__)
int main(int argc, char* argv[]) {
	Application application;
	application.run();
}
#else
#include <Windows.h>
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
	Application application;
	application.run();
}
#endif
