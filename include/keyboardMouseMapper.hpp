#ifndef KEYBOARDMOUSEMAPPER_H
#define KEYBOARDMOUSEMAPPER_H

#include <thread>
#include <atomic>
#include <chrono>
#include "scePadSettings.hpp"

struct TouchpadMouseState {
	float accumX = 0.0f;
	float accumY = 0.0f;
	float smoothedDeltaX = 0.0f;
	float smoothedDeltaY = 0.0f;
	float accumScrollY = 0.0f;

	bool wasTouching = false;
	float lastTouchX = 0.0f;
	float lastTouchY = 0.0f;

	// Click vs Hold state
	bool touchButtonPressed = false;
	bool isHoldingRightClick = false;
	bool isLeftDragging = false;
	bool isTwoFingerClick = false;
	float clickDownX = 0.0f;
	float clickDownY = 0.0f;
	std::chrono::steady_clock::time_point touchDownTime;
};

struct PsBtnState {
	bool isDown = false;
	bool releasePending = false;
	std::chrono::steady_clock::time_point pressTime;
	std::chrono::steady_clock::time_point lastEdgeTime;
};

class KeyboardMouseMapper {
private:
	s_scePadSettings* m_ScePadSettings = nullptr;
	std::atomic<bool> m_ThreadRunning = true;
	std::thread m_thread;
	TouchpadMouseState m_touchState[4] = {};
	PsBtnState m_psState[4] = {};
	void Thread();
	void MoveCursor(int x, int y);
public:
	KeyboardMouseMapper(s_scePadSettings* scePadSettings);
	~KeyboardMouseMapper();
};

#endif // KEYBOARDMOUSEMAPPER_H