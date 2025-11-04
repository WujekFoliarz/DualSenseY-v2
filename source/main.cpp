#include "application.hpp"

#ifdef WINDOWS
#include <Windows.h>
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
	Application application;
	application.Run();
}
#else
int main(int argc, char* argv[]) {
	Application application;
	application.Run();
}
#endif
