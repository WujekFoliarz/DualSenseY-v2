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
#include <client.hpp>
#include <unordered_map>

// User (4) + Peers (4)
static int constexpr VIGEM_CONTROLLER_MAX = 8;

class Vigem {  
private:  
	struct VigemUserData {
		int index;
		Vigem* instance;
	};

#if !defined(__linux__) && !defined(__MACOS__)
   static PVIGEM_CLIENT m_VigemClient;  
   static inline bool m_VigemClientInitalized = false;  

   PVIGEM_TARGET m_360[VIGEM_CONTROLLER_MAX] = {};
   PVIGEM_TARGET m_ds4[VIGEM_CONTROLLER_MAX] = {};

   static VOID CALLBACK xbox360Notification(PVIGEM_CLIENT Client, PVIGEM_TARGET Target, UCHAR LargeMotor, UCHAR SmallMotor, UCHAR LedNumber, LPVOID UserData);
   static VOID CALLBACK ds4Notification(PVIGEM_CLIENT Client, PVIGEM_TARGET Target, UCHAR LargeMotor, UCHAR SmallMotor, DS4_LIGHTBAR_COLOR LightbarColor, LPVOID UserData);

   static VOID CALLBACK x360PeerNotification(PVIGEM_CLIENT Client, PVIGEM_TARGET Target, UCHAR LargeMotor, UCHAR SmallMotor, UCHAR LedNumber, LPVOID UserData);
   static VOID CALLBACK ds4PeerNotification(PVIGEM_CLIENT Client, PVIGEM_TARGET Target, UCHAR LargeMotor, UCHAR SmallMotor, DS4_LIGHTBAR_COLOR LightbarColor, LPVOID UserData);

   std::thread m_VigemThread;
   std::atomic<bool> m_VigemThreadRunning = true;
   VigemUserData m_UserData[4] = { {0,this},{1,this},{2,this},{3,this} };
   void Update360ByTarget(PVIGEM_TARGET Target, s_ScePadData& state);
   void UpdateDs4ByTarget(PVIGEM_TARGET Target, s_ScePadData& state);
   void EmulatedControllerUpdate();
   std::weak_ptr <std::unordered_map<uint32_t, PeerControllerData>> m_PeerControllers;
   std::unordered_map<uint32_t, PVIGEM_TARGET> m_PeerControllerTargets;
#endif

   void applyInputSettingsToScePadState(s_scePadSettings& settings, s_ScePadData& state);
   s_scePadSettings* m_ScePadSettings = nullptr;
   UDP& m_Udp;
   std::atomic<uint32_t> m_SelectedController = 0;
public: 
	Vigem(s_scePadSettings* scePadSettings, UDP& udp);
   ~Vigem();
   void PlugControllerByIndex(uint32_t index, uint32_t controllerType);  
   bool IsVigemConnected(); 
   void SetSelectedController(uint32_t selectedController);
   void SetPeerControllerDataPointer(std::shared_ptr<std::unordered_map<uint32_t, PeerControllerData>> Pointer);
};

#endif // CONTROLLEREMULATION_H