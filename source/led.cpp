#include "led.hpp"
#include <cmath>

void getRainbowColor(float t, s_SceLightBar& led) {
    t = std::fmod(t, 1.0f);
    if (t < 0.0f) t += 1.0f;

    float segment = t * 6.0f;            
    int phase = static_cast<int>(segment) % 6;
    float progress = segment - phase;   

    float r = 0, g = 0, b = 0;

    switch (phase) {
        case 0: r = 1.0f; g = progress; b = 0.0f; break;           
        case 1: r = 1.0f - progress; g = 1.0f; b = 0.0f; break;     
        case 2: r = 0.0f; g = 1.0f; b = progress; break;          
        case 3: r = 0.0f; g = 1.0f - progress; b = 1.0f; break;
        case 4: r = progress; g = 0.0f; b = 1.0f; break;           
        case 5: r = 1.0f; g = 0.0f; b = 1.0f - progress; break;    
    }

    led.r = static_cast<uint8_t>(r * 255.0f);
    led.g = static_cast<uint8_t>(g * 255.0f);
    led.b = static_cast<uint8_t>(b * 255.0f);
}