#ifndef CUSTOMTRIGGERS_H
#define CUSTOMTRIGGERS_H

#include <cstdint>

enum DSXTriggerMode : uint8_t {
	Off = 0x0,
	Rigid = 0x1,
	Pulse = 0x2,
	Rigid_A = 0x1 | 0x20,
	Rigid_B = 0x1 | 0x04,
	Rigid_AB = 0x1 | 0x20 | 0x04,
	Pulse_A = 0x2 | 0x20,
	Pulse_A2 = 35,
	Pulse_B = 0x2 | 0x04,
	Pulse_B2 = 38,
	Pulse_AB = 39,
	Calibration = 0xFC,
	Feedback = 0x21,
	Weapon = 0x25,
	Vibration = 0x26
};

void customTriggerNormal(uint8_t ffb[11]);
void customTriggerGamecube(uint8_t ffb[11]);

#endif // CUSTOMTRIGGERS_H