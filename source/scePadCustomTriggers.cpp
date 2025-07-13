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
