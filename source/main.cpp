#include "application.hpp"

#ifdef WINDOWS
#include <Windows.h>
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
	Application application;
	application.run();
}
#else
int main(int argc, char* argv[]) {
	Application application;
	application.run();
}
#endif
