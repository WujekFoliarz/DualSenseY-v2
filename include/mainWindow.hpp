#ifndef MAINWINDOW_H  
#define MAINWINDOW_H  

#include "scePadSettings.hpp"  
#include "strings.hpp"  
#include "audioPassthrough.hpp"  
#include <controllerEmulation.hpp>

class MainWindow {  
   Strings& m_strings;  
   AudioPassthrough& m_audio;  
   Vigem& m_vigem;
private:  
   bool about(bool* open);  
   bool menuBar();  
   bool controllers(int& currentController, s_scePadSettings scePadSettings[4], float scale);  
   bool led(int& currentController, s_scePadSettings scePadSettings[4], float scale);  
   bool udp(int& currentController, float scale);  
   bool audio(int& currentController, s_scePadSettings scePadSettings[4]);
   bool emulation(int& currentController, s_scePadSettings scePadSettings[4], Vigem& vigem);
public:  
   MainWindow(Strings& strings, AudioPassthrough& audio, Vigem& vigem)  
       : m_strings(strings), m_audio(audio), m_vigem(vigem) {}
   void show(s_scePadSettings scePadSettings[4], float scale);  
};  

#endif // MAINWINDOW_H