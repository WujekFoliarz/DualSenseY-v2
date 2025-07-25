#ifndef MAINWINDOW_H  
#define MAINWINDOW_H  

#include "scePadSettings.hpp"  
#include "strings.hpp"  
#include "audioPassthrough.hpp"  
#include "udp.hpp"
#include "controllerEmulation.hpp"
#include "utils.hpp"

class MainWindow {  
   Strings& m_strings;  
   AudioPassthrough& m_audio;  
   Vigem& m_vigem;
   UDP& m_udp;
   bool m_isAdminWindows = isRunningAsAdministratorWindows();
private:  
   int m_selectedController = 0;
   bool about(bool* open);  
   bool menuBar();  
   bool controllers(int& currentController, s_scePadSettings scePadSettings[4], float scale);  
   bool led(int& currentController, s_scePadSettings scePadSettings[4], float scale);  
   bool udp(int& currentController, float scale);  
   bool audio(int& currentController, s_scePadSettings scePadSettings[4]);
   bool emulation(int& currentController, s_scePadSettings scePadSettings[4]);
   bool adaptiveTriggers(int& currentController, s_scePadSettings scePadSettings[4]);
public:  
   MainWindow(Strings& strings, AudioPassthrough& audio, Vigem& vigem, UDP& udp)  
       : m_strings(strings), m_audio(audio), m_vigem(vigem), m_udp(udp) {}
   void show(s_scePadSettings scePadSettings[4], float scale);
   int getSelectedController();
};  

#endif // MAINWINDOW_H