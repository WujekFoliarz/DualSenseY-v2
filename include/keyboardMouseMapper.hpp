#ifndef KEYBOARDMOUSEMAPPER_H
#define KEYBOARDMOUSEMAPPER_H

#include <thread>
#include <atomic>
#include "scePadSettings.hpp"

class KeyboardMouseMapper {
private:
	s_scePadSettings* m_scePadSettings = nullptr;
	std::atomic<bool> m_threadRunning = true;
	std::thread m_thread;
	void thread();
public:
	KeyboardMouseMapper(s_scePadSettings* scePadSettings);
	~KeyboardMouseMapper();
};

#endif // KEYBOARDMOUSEMAPPER_H