#ifndef CUSTOMTRIGGERS_H
#define CUSTOMTRIGGERS_H

#include <cstdint>
#include <vector>

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
void customTriggerVerySoft(uint8_t ffb[11]);
void customTriggerSoft(uint8_t ffb[11]);
void customTriggerHard(uint8_t ffb[11]);
void customTriggerVeryHard(uint8_t ffb[11]);
void customTriggerHardest(uint8_t ffb[11]);
void customTriggerRigid(uint8_t ffb[11]);
void customTriggerVibrateTrigger(uint8_t ffb[11]);
void customTriggerChoppy(uint8_t ffb[11]);
void customTriggerMedium(uint8_t ffb[11]);
void customTriggerVibrateTriggerPulse(uint8_t ffb[11]);
void customTriggerCustomTriggerValue(std::vector<uint8_t> param, uint8_t ffb[11]);
void customTriggerResistance(std::vector<uint8_t> param, uint8_t ffb[11]);
void customTriggerBow(std::vector<uint8_t> param, uint8_t ffb[11]);
void customTriggerGalloping(std::vector<uint8_t> param, uint8_t ffb[11]);
void customTriggerSemiAutomaticGun(std::vector<uint8_t> param, uint8_t ffb[11]);
void customTriggerAutomaticGun(std::vector<uint8_t> param, uint8_t ffb[11]);
void customTriggerMachine(std::vector<uint8_t> param, uint8_t ffb[11]);
void customTriggerVIBRATE_TRIGGER_10Hz(std::vector<uint8_t> param, uint8_t ffb[11]);



#endif // CUSTOMTRIGGERS_H