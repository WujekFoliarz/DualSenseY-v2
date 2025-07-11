#ifndef CONTROLLEREMULATION_H  
#define CONTROLLEREMULATION_H  

#if !defined(__linux__) && !defined(__MACOS__)

#define WIN32_LEAN_AND_MEAN  
#include <windows.h>  
#include <Xinput.h>  
#include <ViGEm/Client.h>  
#pragma comment(lib, "setupapi.lib")  
#include <timeapi.h>
#pragma comment(lib, "winmm.lib")

#endif

#include <cstdint>
#include "scePadSettings.hpp" 
#include "scePadHandle.hpp"

class Vigem {  
private:  
#if !defined(__linux__) && !defined(__MACOS__)
   static PVIGEM_CLIENT m_vigemClient;  
   static inline bool m_vigemClientInitalized = false;  

   PVIGEM_TARGET m_360[4] = {};  
   PVIGEM_TARGET m_ds4[4] = {};
#endif

   void update360ByIndex(uint32_t index, s_ScePadData& state);
   void updateDs4ByIndex(uint32_t index, s_ScePadData& state);

   friend inline void emulatedControllerUpdate(Vigem& vigem, s_scePadSettings scePadSettings[4], std::atomic<bool>& threadRunning);
public:  
   Vigem();  
   void plugControllerByIndex(uint32_t index, uint32_t controllerType);  
   bool isVigemConnected();  
};  

inline void emulatedControllerUpdate(Vigem& vigem, s_scePadSettings scePadSettings[4], std::atomic<bool>& threadRunning) {
#if !defined(__linux__) && !defined(__MACOS__)
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	timeBeginPeriod(1);

	while (threadRunning) {
		for (uint32_t i = 0; i < 4; i++) {
		
			if ((EmulatedController)scePadSettings[i].emulatedController != EmulatedController::NONE) {
				s_ScePadData scePadState = {};
				uint32_t result = scePadReadState(g_scePad[i], &scePadState);

				if (result == SCE_OK) {

					if ((EmulatedController)scePadSettings[i].emulatedController == EmulatedController::XBOX360) {
						vigem.update360ByIndex(i, scePadState);
					}
					else if ((EmulatedController)scePadSettings[i].emulatedController == EmulatedController::DUALSHOCK4) {
						vigem.updateDs4ByIndex(i, scePadState);
					}
				}
			}
		}

		std::this_thread::sleep_for(std::chrono::nanoseconds(300));
	}
#endif
}

#endif // CONTROLLEREMULATION_H