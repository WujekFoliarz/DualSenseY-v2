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
#include <atomic>
#include <thread>
#include "udp.hpp"

class Vigem {  
private:  
#if !defined(__linux__) && !defined(__MACOS__)
   static PVIGEM_CLIENT m_vigemClient;  
   static inline bool m_vigemClientInitalized = false;  

   PVIGEM_TARGET m_360[4] = {};  
   PVIGEM_TARGET m_ds4[4] = {};
#endif
 
   std::thread m_vigemThread;
   std::atomic<bool> m_vigemThreadRunning = true;
   s_scePadSettings* m_scePadSettings = nullptr;
   UDP& m_udp;
   std::atomic<uint32_t> m_selectedController = 0;
   void update360ByIndex(uint32_t index, s_ScePadData& state);
   void updateDs4ByIndex(uint32_t index, s_ScePadData& state);
   void applyInputSettingsToScePadState(s_scePadSettings& settings, s_ScePadData& state);
   void emulatedControllerUpdate();

public: 
	Vigem(s_scePadSettings* scePadSettings, UDP& udp);
   ~Vigem();
   void plugControllerByIndex(uint32_t index, uint32_t controllerType);  
   bool isVigemConnected(); 
   void setSelectedController(uint32_t selectedController);
};

#endif // CONTROLLEREMULATION_H