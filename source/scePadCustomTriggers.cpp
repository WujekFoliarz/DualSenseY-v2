#include "scePadCustomTriggers.hpp"

void customTriggerNormal(uint8_t ffb[11]) {
	ffb[0] = DSXTriggerMode::Rigid_B;
	ffb[1] = 0;
	ffb[2] = 0;
	ffb[3] = 0;
	ffb[4] = 0;
	ffb[5] = 0;
	ffb[6] = 0;
	ffb[7] = 0;
	ffb[8] = 0;
	ffb[9] = 0;
	ffb[10] = 0;
}

void customTriggerGamecube(uint8_t ffb[11]) {
	ffb[0] = DSXTriggerMode::Pulse;
	ffb[1] = 144;
	ffb[2] = 160;
	ffb[3] = 255;
	ffb[4] = 0;
	ffb[5] = 0;
	ffb[6] = 0;
	ffb[7] = 0;
	ffb[8] = 0;
	ffb[9] = 0;
	ffb[10] = 0;
}

void customTriggerVerySoft(uint8_t ffb[11]) {
	ffb[0] = DSXTriggerMode::Pulse;
	ffb[1] = 128;
	ffb[2] = 160;
	ffb[3] = 255;
	ffb[4] = 0;
	ffb[5] = 0;
	ffb[6] = 0;
	ffb[7] = 0;
	ffb[8] = 0;
	ffb[9] = 0;
	ffb[10] = 0;
}

void customTriggerSoft(uint8_t ffb[11]) {
	ffb[0] = DSXTriggerMode::Rigid_A;
	ffb[1] = 69;
	ffb[2] = 160;
	ffb[3] = 255;
	ffb[4] = 0;
	ffb[5] = 0;
	ffb[6] = 0;
	ffb[7] = 0;
	ffb[8] = 0;
	ffb[9] = 0;
	ffb[10] = 0;
}

void customTriggerHard(uint8_t ffb[11]) {
	ffb[0] = DSXTriggerMode::Rigid_A;
	ffb[1] = 32;
	ffb[2] = 160;
	ffb[3] = 255;
	ffb[4] = 255;
	ffb[5] = 255;
	ffb[6] = 255;
	ffb[7] = 255;
	ffb[8] = 0;
	ffb[9] = 0;
	ffb[10] = 0;
}

void customTriggerVeryHard(uint8_t ffb[11]) {
	ffb[0] = DSXTriggerMode::Rigid_A;
	ffb[1] = 16;
	ffb[2] = 160;
	ffb[3] = 255;
	ffb[4] = 255;
	ffb[5] = 255;
	ffb[6] = 255;
	ffb[7] = 255;
	ffb[8] = 0;
	ffb[9] = 0;
	ffb[10] = 0;
}

void customTriggerHardest(uint8_t ffb[11]) {
	ffb[0] = DSXTriggerMode::Pulse;
	ffb[1] = 0;
	ffb[2] = 255;
	ffb[3] = 255;
	ffb[4] = 255;
	ffb[5] = 255;
	ffb[6] = 255;
	ffb[7] = 255;
	ffb[8] = 0;
	ffb[9] = 0;
	ffb[10] = 0;
}

void customTriggerRigid(uint8_t ffb[11]) {
	ffb[0] = DSXTriggerMode::Rigid;
	ffb[1] = 0;
	ffb[2] = 255;
	ffb[3] = 0;
	ffb[4] = 0;
	ffb[5] = 0;
	ffb[6] = 0;
	ffb[7] = 0;
	ffb[8] = 0;
	ffb[9] = 0;
	ffb[10] = 0;
}

void customTriggerVibrateTrigger(uint8_t ffb[11]) {
	ffb[0] = DSXTriggerMode::Pulse_AB;
	ffb[1] = 37;
	ffb[2] = 35;
	ffb[3] = 6;
	ffb[4] = 39;
	ffb[5] = 33;
	ffb[6] = 35;
	ffb[7] = 34;
	ffb[8] = 0;
	ffb[9] = 0;
	ffb[10] = 0;
}

void customTriggerChoppy(uint8_t ffb[11]) {
	ffb[0] = DSXTriggerMode::Rigid_A;
	ffb[1] = 2;
	ffb[2] = 39;
	ffb[3] = 33;
	ffb[4] = 39;
	ffb[5] = 38;
	ffb[6] = 2;
	ffb[7] = 0;
	ffb[8] = 0;
	ffb[9] = 0;
	ffb[10] = 0;
}

void customTriggerMedium(uint8_t ffb[11]) {
	ffb[0] = DSXTriggerMode::Pulse_A;
	ffb[1] = 2;
	ffb[2] = 35;
	ffb[3] = 1;
	ffb[4] = 6;
	ffb[5] = 6;
	ffb[6] = 1;
	ffb[7] = 33;
	ffb[8] = 0;
	ffb[9] = 0;
	ffb[10] = 0;
}

void customTriggerVibrateTriggerPulse(uint8_t ffb[11]) {
	ffb[0] = DSXTriggerMode::Pulse_AB;
	ffb[1] = 37;
	ffb[2] = 35;
	ffb[3] = 6;
	ffb[4] = 39;
	ffb[5] = 33;
	ffb[6] = 35;
	ffb[7] = 34;
	ffb[8] = 0;
	ffb[9] = 0;
	ffb[10] = 0;
}

static uint8_t getOrZero(const std::vector<uint8_t>& vec, size_t index) {
	return index < vec.size() ? vec[index] : 0;
}

void customTriggerCustomTriggerValue(std::vector<uint8_t> param, uint8_t ffb[11]) {
	if (param.size() < 1) return;

	switch (param[0]) {
		case 0: ffb[0] = DSXTriggerMode::Off; break;
		case 1: ffb[0] = DSXTriggerMode::Rigid; break;
		case 2: ffb[0] = DSXTriggerMode::Rigid_A; break;
		case 3: ffb[0] = DSXTriggerMode::Rigid_B; break;
		case 4: ffb[0] = DSXTriggerMode::Rigid_AB; break;
		case 5: ffb[0] = DSXTriggerMode::Pulse; break;
		case 6: ffb[0] = DSXTriggerMode::Pulse_A; break;
		case 7: ffb[0] = DSXTriggerMode::Pulse_B; break;
		case 8: ffb[0] = DSXTriggerMode::Pulse_AB; break;
		case 9: ffb[0] = DSXTriggerMode::Pulse_B; break;
		case 10: ffb[0] = DSXTriggerMode::Pulse_B2; break;
		case 11: ffb[0] = DSXTriggerMode::Pulse_B; break;
		case 12: ffb[0] = DSXTriggerMode::Pulse_B2; break;
		case 13: ffb[0] = DSXTriggerMode::Pulse_AB; break;
		case 14: ffb[0] = DSXTriggerMode::Pulse_AB; break;
		case 15: ffb[0] = DSXTriggerMode::Pulse_AB; break;
		case 16: ffb[0] = DSXTriggerMode::Pulse_AB; break;
	}

	ffb[1] = getOrZero(param, 1);
	ffb[2] = getOrZero(param, 2);
	ffb[3] = getOrZero(param, 3);
	ffb[4] = getOrZero(param, 4);
	ffb[5] = getOrZero(param, 5);
	ffb[6] = getOrZero(param, 6);
	ffb[7] = getOrZero(param, 7);
	ffb[8] = 0;
	ffb[9] = 0;
	ffb[10] = getOrZero(param, 8);
}

void customTriggerResistance(std::vector<uint8_t> param, uint8_t ffb[11]) {
	if (param.size() < 2) return;

	ffb[0] = DSXTriggerMode::Feedback;
	uint8_t start = param[0]; 
	uint8_t force = param[1];
	if (start <= 9 && force <= 8 && force > 0) {
		uint8_t b = static_cast<uint8_t>((force - 1) & 7);
		uint32_t num = 0;
		uint16_t num2 = 0;
		for (int i = static_cast<int>(start); i < 10; ++i) {
			num |= (static_cast<uint32_t>(b) << (3 * i));
			num2 |= (1 << i);
		}
		ffb[1] = static_cast<uint8_t>(num2 & 0xFF);
		ffb[2] = static_cast<uint8_t>((num2 >> 8) & 0xFF);
		ffb[3] = static_cast<uint8_t>(num & 0xFF);
		ffb[4] = static_cast<uint8_t>((num >> 8) & 0xFF);
		ffb[5] = static_cast<uint8_t>((num >> 16) & 0xFF);
		ffb[6] = static_cast<uint8_t>((num >> 24) & 0xFF);
		ffb[7] = 0; 
		ffb[8] = 0; 
		ffb[9] = 0;
		ffb[10] = 0;
	}
}

void customTriggerBow(std::vector<uint8_t> param, uint8_t ffb[11]) {
	if (param.size() < 4) return;

	ffb[0] = DSXTriggerMode::Pulse_A;
	uint8_t start = param[0];
	uint8_t end = param[1];
	uint8_t force = param[2];
	uint8_t snapForce = param[3];
	if (start <= 8 && end <= 8 && start < end && force <= 8 && snapForce <= 8 && end > 0 && force > 0 && snapForce > 0) {
		uint16_t num = static_cast<uint16_t>((1 << start) | (1 << end));
		uint32_t num2 = static_cast<uint32_t>(((force - 1) & 7) | (((snapForce - 1) & 7) << 3));
		ffb[1] = static_cast<uint8_t>(num & 0xFF);
		ffb[2] = static_cast<uint8_t>((num >> 8) & 0xFF);
		ffb[3] = static_cast<uint8_t>(num2 & 0xFF);
		ffb[4] = static_cast<uint8_t>((num2 >> 8) & 0xFF);
		ffb[5] = 0;
		ffb[6] = 0;
		ffb[7] = 0;
		ffb[8] = 0;
		ffb[9] = 0;
		ffb[10] = 0;
	}
}

void customTriggerGalloping(std::vector<uint8_t> param, uint8_t ffb[11]) {
	if (param.size() < 5) return;

	ffb[0] = DSXTriggerMode::Pulse_A2;
	uint8_t start = param[0];
	uint8_t end = param[1];
	uint8_t firstFoot = param[2];
	uint8_t secondFoot = param[3];
	uint8_t frequency = param[4];
	if (start <= 8 && end <= 9 && start < end && secondFoot <= 7 && firstFoot <= 6 && firstFoot < secondFoot && frequency > 0) {
		uint16_t num = static_cast<uint16_t>((1 << start) | (1 << end));
		uint32_t num2 = static_cast<uint32_t>((secondFoot & 7) | ((firstFoot & 7) << 3));
		ffb[1] = static_cast<uint8_t>(num & 0xFF);
		ffb[2] = static_cast<uint8_t>((num >> 8) & 0xFF);
		ffb[3] = static_cast<uint8_t>(num2 & 0xFF);
		ffb[4] = frequency;
		ffb[5] = 0; 
		ffb[6] = 0; 
		ffb[7] = 0;
		ffb[8] = 0; 
		ffb[9] = 0; 
		ffb[10] = 0;
	}
}

void customTriggerSemiAutomaticGun(std::vector<uint8_t> param, uint8_t ffb[11]) {
	if (param.size() < 3) return;

	ffb[0] = DSXTriggerMode::Rigid_AB;
	uint8_t start = param[0];
	uint8_t end = param[1];
	uint8_t force = param[2];
	if (start <= 7 && start >= 2 && end <= 8 && end > start && force <= 8 && force > 0) {
		uint16_t num = static_cast<uint16_t>((1 << start) | (1 << end));
		ffb[1] = static_cast<uint8_t>(num & 0xFF);
		ffb[2] = static_cast<uint8_t>((num >> 8) & 0xFF);
		ffb[3] = force - 1;
		ffb[4] = 0; 
		ffb[5] = 0; 
		ffb[6] = 0;
		ffb[7] = 0; 
		ffb[8] = 0; 
		ffb[9] = 0;
		ffb[10] = 0; 
	}
}

void customTriggerAutomaticGun(std::vector<uint8_t> param, uint8_t ffb[11]) {
	if (param.size() < 3) return;

	ffb[0] = DSXTriggerMode::Pulse_B2;
	uint8_t start = param[0];
	uint8_t strength = param[1];
	uint8_t frequency = param[2];
	if (start <= 9 && strength <= 8 && strength > 0 && frequency > 0) {
		uint8_t b = (strength - 1) & 7;
		uint32_t num = 0; uint16_t num2 = 0;
		for (int i = static_cast<int>(start); i < 10; i++) {
			num |= static_cast<uint32_t>(b) << (3 * i);
			num2 |= static_cast<uint16_t>(1 << i);
		}
		ffb[1] = static_cast<uint8_t>(num2 & 0xFF);
		ffb[2] = static_cast<uint8_t>((num2 >> 8) & 0xFF);
		ffb[3] = static_cast<uint8_t>(num & 0xFF);
		ffb[4] = static_cast<uint8_t>((num >> 8) & 0xFF);
		ffb[5] = static_cast<uint8_t>((num >> 16) & 0xFF);
		ffb[6] = static_cast<uint8_t>((num >> 24) & 0xFF);
		ffb[7] = 0; 
		ffb[8] = 0; 
		ffb[9] = frequency;
		ffb[10] = 0;
	}
}

void customTriggerMachine(std::vector<uint8_t> param, uint8_t ffb[11]) {
	if (param.size() < 6) return;

	ffb[0] = DSXTriggerMode::Pulse_AB;
	uint8_t start = param[0];
	uint8_t end = param[1];
	uint8_t strengthA = param[2];
	uint8_t strengthB = param[3];
	uint8_t frequency = param[4];
	uint8_t period = param[5];
	if (start <= 8 && end <= 9 && end > start && strengthA <= 7 && strengthB <= 7 && frequency > 0) {
		uint16_t num = static_cast<uint16_t>((1 << start) | (1 << end));
		uint32_t num2 = static_cast<uint32_t>((strengthA & 7) | ((strengthB & 7) << 3));
		ffb[1] = static_cast<uint8_t>(num & 0xFF);
		ffb[2] = static_cast<uint8_t>((num >> 8) & 0xFF);
		ffb[3] = static_cast<uint8_t>(num2 & 0xFF);
		ffb[4] = frequency; 
		ffb[5] = period;
		ffb[6] = 0;
		ffb[7] = 0; 
		ffb[8] = 0;
		ffb[9] = 0; 
		ffb[10] = 0;
	}
}

void customTriggerVIBRATE_TRIGGER_10Hz(std::vector<uint8_t> param, uint8_t ffb[11]) {
	ffb[0] = DSXTriggerMode::Pulse_B;
	ffb[1] = 10;
	ffb[2] = 255;
	ffb[3] = 40;
	ffb[4] = 0;
	ffb[5] = 0;
	ffb[6] = 0;
	ffb[7] = 0;
	ffb[8] = 0;
	ffb[9] = 0;
	ffb[10] = 0;
}

void customTriggerOFF(uint8_t ffb[11]) {
	ffb[0] = DSXTriggerMode::Pulse_B;
	ffb[1] = 0;
	ffb[2] = 0;
	ffb[3] = 0;
	ffb[4] = 0;
	ffb[5] = 0;
	ffb[6] = 0;
	ffb[7] = 0;
	ffb[8] = 0;
	ffb[9] = 0;
	ffb[10] = 0;
}
