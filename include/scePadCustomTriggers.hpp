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

void CustomTriggerNormal(uint8_t ffb[11]);
void CustomTriggerGamecube(uint8_t ffb[11]);
void CustomTriggerVerySoft(uint8_t ffb[11]);
void CustomTriggerSoft(uint8_t ffb[11]);
void CustomTriggerHard(uint8_t ffb[11]);
void CustomTriggerVeryHard(uint8_t ffb[11]);
void CustomTriggerHardest(uint8_t ffb[11]);
void CustomTriggerRigid(uint8_t ffb[11]);
void CustomTriggerVibrateTrigger(uint8_t ffb[11]);
void CustomTriggerChoppy(uint8_t ffb[11]);
void CustomTriggerMedium(uint8_t ffb[11]);
void CustomTriggerVibrateTriggerPulse(uint8_t ffb[11]);
void CustomTriggerCustomTriggerValue(std::vector<uint8_t> param, uint8_t ffb[11]);
void CustomTriggerResistance(std::vector<uint8_t> param, uint8_t ffb[11]);
void CustomTriggerBow(std::vector<uint8_t> param, uint8_t ffb[11]);
void CustomTriggerGalloping(std::vector<uint8_t> param, uint8_t ffb[11]);
void CustomTriggerSemiAutomaticGun(std::vector<uint8_t> param, uint8_t ffb[11]);
void CustomTriggerAutomaticGun(std::vector<uint8_t> param, uint8_t ffb[11]);
void CustomTriggerMachine(std::vector<uint8_t> param, uint8_t ffb[11]);
void CustomTriggerBetterVibration(std::vector<uint8_t> param, uint8_t ffb[11]);
void CustomTriggerVIBRATE_TRIGGER_10Hz(std::vector<uint8_t> param, uint8_t ffb[11]);
void CustomTriggerOFF(uint8_t ffb[11]);

#endif // CUSTOMTRIGGERS_H