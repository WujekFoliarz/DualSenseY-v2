#include "application.hpp"
#include <iostream>

#if defined(WINDOWS)
#include <Windows.h>
#endif

int main(int argc, char* argv[]) {
	if (argc < 3) {
		std::cout << "Not enough arguments!" << std::endl;
		getchar();
		return -1;
	}

	Application application;

	application.Run(std::string(argv[1]), std::string(argv[2]));
}