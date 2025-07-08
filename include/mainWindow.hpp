#ifndef MAINWINDOW_H  
#define MAINWINDOW_H  

#include "scePadSettings.h"  
#include "strings.hpp"  
#include "audioPassthrough.hpp"  

class MainWindow {  
   Strings& m_strings;  
   AudioPassthrough& m_audio;  
private:  
   bool about(bool* open);  
   bool menuBar();  
   bool controllers(int& currentController, s_scePadSettings scePadSettings[4], float scale);  
   bool led(int& currentController, s_scePadSettings scePadSettings[4], float scale);  
   bool udp(int& currentController, float scale);  
   bool audio(int& currentController, s_scePadSettings scePadSettings[4]);
public:  
   MainWindow(Strings& strings, AudioPassthrough& audio)  
       : m_strings(strings), m_audio(audio) {}  
   void show(s_scePadSettings scePadSettings[4], float scale);  
};  

#endif // MAINWINDOW_H