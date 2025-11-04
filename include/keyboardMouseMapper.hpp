#ifndef KEYBOARDMOUSEMAPPER_H
#define KEYBOARDMOUSEMAPPER_H

#include <thread>
#include <atomic>
#include "scePadSettings.hpp"

class KeyboardMouseMapper {
private:
	s_scePadSettings* m_ScePadSettings = nullptr;
	std::atomic<bool> m_ThreadRunning = true;
	std::thread m_thread;
	void Thread();
	void MoveCursor(int x, int y);
public:
	KeyboardMouseMapper(s_scePadSettings* scePadSettings);
	~KeyboardMouseMapper();
};

#endif // KEYBOARDMOUSEMAPPER_H