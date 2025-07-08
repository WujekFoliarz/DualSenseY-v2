#include "crc.h"

uint32_t compute(unsigned char* buffer, size_t len) {
	uint32_t result = crcSeed;
	for (size_t i = 0; i < len; i++) {
		result = hashTable[((unsigned char)result) ^ ((unsigned char)buffer[i])] ^
			(result >> 8);
	}
	return result;
}
