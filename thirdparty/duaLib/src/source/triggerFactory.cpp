#include "triggerFactory.h"
#include <cmath>
#include <algorithm>

/*
 * MIT License
 *
 * Copyright (c) 2021-2022 John "Nielk1" Klein
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

bool TriggerEffectGenerator::Off(uint8_t(&forces)[11], size_t destinationIndex) {
	if (destinationIndex + 10 >= 11) return false;
	forces[destinationIndex + 0] = static_cast<uint8_t>(TriggerEffectType::Off);
	forces[destinationIndex + 1] = 0x00;
	forces[destinationIndex + 2] = 0x00;
	forces[destinationIndex + 3] = 0x00;
	forces[destinationIndex + 4] = 0x00;
	forces[destinationIndex + 5] = 0x00;
	forces[destinationIndex + 6] = 0x00;
	forces[destinationIndex + 7] = 0x00;
	forces[destinationIndex + 8] = 0x00;
	forces[destinationIndex + 9] = 0x00;
	forces[destinationIndex + 10] = 0x00;
	return true;
}

bool TriggerEffectGenerator::Feedback(uint8_t(&forces)[11], size_t destinationIndex, uint8_t position, uint8_t strength) {
	if (destinationIndex + 10 >= 11) return false;
	if (position > 9 || strength > 8) return false;
	if (strength > 0) {
		uint8_t forceValue = (strength - 1) & 0x07;
		uint32_t forceZones = 0;
		uint16_t activeZones = 0;
		for (int i = position; i < 10; i++) {
			forceZones |= static_cast<uint32_t>(forceValue << (3 * i));
			activeZones |= static_cast<uint16_t>(1 << i);
		}
		forces[destinationIndex + 0] = static_cast<uint8_t>(TriggerEffectType::Feedback);
		forces[destinationIndex + 1] = static_cast<uint8_t>((activeZones >> 0) & 0xff);
		forces[destinationIndex + 2] = static_cast<uint8_t>((activeZones >> 8) & 0xff);
		forces[destinationIndex + 3] = static_cast<uint8_t>((forceZones >> 0) & 0xff);
		forces[destinationIndex + 4] = static_cast<uint8_t>((forceZones >> 8) & 0xff);
		forces[destinationIndex + 5] = static_cast<uint8_t>((forceZones >> 16) & 0xff);
		forces[destinationIndex + 6] = static_cast<uint8_t>((forceZones >> 24) & 0xff);
		forces[destinationIndex + 7] = 0x00;
		forces[destinationIndex + 8] = 0x00;
		forces[destinationIndex + 9] = 0x00;
		forces[destinationIndex + 10] = 0x00;
		return true;
	}
	return Off(forces, destinationIndex);
}

bool TriggerEffectGenerator::Weapon(uint8_t(&forces)[11], size_t destinationIndex, uint8_t startPosition, uint8_t endPosition, uint8_t strength) {
	if (destinationIndex + 10 >= 11) return false;
	if (startPosition > 7 || startPosition < 2 || endPosition > 8 || endPosition <= startPosition || strength > 8) return false;
	if (strength > 0) {
		uint16_t startAndStopZones = static_cast<uint16_t>((1 << startPosition) | (1 << endPosition));
		forces[destinationIndex + 0] = static_cast<uint8_t>(TriggerEffectType::Weapon);
		forces[destinationIndex + 1] = static_cast<uint8_t>((startAndStopZones >> 0) & 0xff);
		forces[destinationIndex + 2] = static_cast<uint8_t>((startAndStopZones >> 8) & 0xff);
		forces[destinationIndex + 3] = strength - 1;
		forces[destinationIndex + 4] = 0x00;
		forces[destinationIndex + 5] = 0x00;
		forces[destinationIndex + 6] = 0x00;
		forces[destinationIndex + 7] = 0x00;
		forces[destinationIndex + 8] = 0x00;
		forces[destinationIndex + 9] = 0x00;
		forces[destinationIndex + 10] = 0x00;
		return true;
	}
	return Off(forces, destinationIndex);
}

bool TriggerEffectGenerator::Vibration(uint8_t(&forces)[11], size_t destinationIndex, uint8_t position, uint8_t amplitude, uint8_t frequency) {
	if (destinationIndex + 10 >= 11) return false;
	if (position > 9 || amplitude > 8) return false;
	if (amplitude > 0 && frequency > 0) {
		uint8_t strengthValue = (amplitude - 1) & 0x07;
		uint32_t amplitudeZones = 0;
		uint16_t activeZones = 0;
		for (int i = position; i < 10; i++) {
			amplitudeZones |= static_cast<uint32_t>(strengthValue << (3 * i));
			activeZones |= static_cast<uint16_t>(1 << i);
		}
		forces[destinationIndex + 0] = static_cast<uint8_t>(TriggerEffectType::Vibration);
		forces[destinationIndex + 1] = static_cast<uint8_t>((activeZones >> 0) & 0xff);
		forces[destinationIndex + 2] = static_cast<uint8_t>((activeZones >> 8) & 0xff);
		forces[destinationIndex + 3] = static_cast<uint8_t>((amplitudeZones >> 0) & 0xff);
		forces[destinationIndex + 4] = static_cast<uint8_t>((amplitudeZones >> 8) & 0xff);
		forces[destinationIndex + 5] = static_cast<uint8_t>((amplitudeZones >> 16) & 0xff);
		forces[destinationIndex + 6] = static_cast<uint8_t>((amplitudeZones >> 24) & 0xff);
		forces[destinationIndex + 7] = 0x00;
		forces[destinationIndex + 8] = 0x00;
		forces[destinationIndex + 9] = frequency;
		forces[destinationIndex + 10] = 0x00;
		return true;
	}
	return Off(forces, destinationIndex);
}

bool TriggerEffectGenerator::MultiplePositionFeedback(uint8_t(&forces)[11], size_t destinationIndex, uint8_t(&strength)[10]) {
	if (destinationIndex + 10 < 11) {
		uint32_t forceZones = 0;
		uint16_t activeZones = 0;
		for (int i = 0; i < 10; i++) {
			if (strength[i] > 0) {
				uint8_t forceValue = (strength[i] - 1) & 0x07;
				forceZones |= static_cast<uint32_t>(forceValue << (3 * i));
				activeZones |= static_cast<uint16_t>(1 << i);
			}
		}
		forces[destinationIndex + 0] = static_cast<uint8_t>(TriggerEffectType::Feedback);
		forces[destinationIndex + 1] = static_cast<uint8_t>((activeZones >> 0) & 0xff);
		forces[destinationIndex + 2] = static_cast<uint8_t>((activeZones >> 8) & 0xff);
		forces[destinationIndex + 3] = static_cast<uint8_t>((forceZones >> 0) & 0xff);
		forces[destinationIndex + 4] = static_cast<uint8_t>((forceZones >> 8) & 0xff);
		forces[destinationIndex + 5] = static_cast<uint8_t>((forceZones >> 16) & 0xff);
		forces[destinationIndex + 6] = static_cast<uint8_t>((forceZones >> 24) & 0xff);
		forces[destinationIndex + 7] = 0x00;
		forces[destinationIndex + 8] = 0x00;
		forces[destinationIndex + 9] = 0x00;
		forces[destinationIndex + 10] = 0x00;
		return true;
	}

	return Off(forces, destinationIndex);
}

bool TriggerEffectGenerator::SlopeFeedback(uint8_t(&forces)[11], size_t destinationIndex, uint8_t startPosition, uint8_t endPosition, uint8_t startStrength, uint8_t endStrength) {
	if (destinationIndex + 10 >= 11) return false;
	if (startPosition > 8 || endPosition > 9 || endPosition <= startPosition || startStrength > 8 || startStrength < 1 || endStrength > 8 || endStrength < 1) return false;
	uint8_t strength[10];
	float slope = static_cast<float>(endStrength - startStrength) / static_cast<float>(endPosition - startPosition);
	for (int i = startPosition; i < 10; i++) {
		if (i <= endPosition) {
			strength[i] = static_cast<uint8_t>(std::round(startStrength + slope * (i - startPosition)));
		}
		else {
			strength[i] = endStrength;
		}
	}
	return MultiplePositionFeedback(forces, destinationIndex, strength);
}

bool TriggerEffectGenerator::MultiplePositionVibration(uint8_t(&forces)[11], size_t destinationIndex, uint8_t frequency, uint8_t(&amplitude)[10]) {
	if (destinationIndex + 10 < 11)
	{
		uint32_t strengthZones = 0;
		uint16_t activeZones = 0;
		for (int i = 0; i < 10; i++) {
			if (amplitude[i] > 0) {
				uint8_t strengthValue = (amplitude[i] - 1) & 0x07;
				strengthZones |= static_cast<uint32_t>(strengthValue << (3 * i));
				activeZones |= static_cast<uint16_t>(1 << i);
			}
		}
		forces[destinationIndex + 0] = static_cast<uint8_t>(TriggerEffectType::Vibration);
		forces[destinationIndex + 1] = static_cast<uint8_t>((activeZones >> 0) & 0xff);
		forces[destinationIndex + 2] = static_cast<uint8_t>((activeZones >> 8) & 0xff);
		forces[destinationIndex + 3] = static_cast<uint8_t>((strengthZones >> 0) & 0xff);
		forces[destinationIndex + 4] = static_cast<uint8_t>((strengthZones >> 8) & 0xff);
		forces[destinationIndex + 5] = static_cast<uint8_t>((strengthZones >> 16) & 0xff);
		forces[destinationIndex + 6] = static_cast<uint8_t>((strengthZones >> 24) & 0xff);
		forces[destinationIndex + 7] = 0x00;
		forces[destinationIndex + 8] = 0x00;
		forces[destinationIndex + 9] = frequency;
		forces[destinationIndex + 10] = 0x00;
		return true;
	}
	return Off(forces, destinationIndex);
}

bool TriggerEffectGenerator::Bow(uint8_t(&forces)[11], size_t destinationIndex, uint8_t startPosition, uint8_t endPosition, uint8_t strength, uint8_t snapForce) {
	if (destinationIndex + 10 >= 11) return false;
	if (startPosition > 8 || endPosition > 8 || startPosition >= endPosition || strength > 8 || snapForce > 8) return false;
	if (endPosition > 0 && strength > 0 && snapForce > 0) {
		uint16_t startAndStopZones = static_cast<uint16_t>((1 << startPosition) | (1 << endPosition));
		uint32_t forcePair = static_cast<uint32_t>((((strength - 1) & 0x07) << (3 * 0)) | (((snapForce - 1) & 0x07) << (3 * 1)));
		forces[destinationIndex + 0] = static_cast<uint8_t>(TriggerEffectType::Bow);
		forces[destinationIndex + 1] = static_cast<uint8_t>((startAndStopZones >> 0) & 0xff);
		forces[destinationIndex + 2] = static_cast<uint8_t>((startAndStopZones >> 8) & 0xff);
		forces[destinationIndex + 3] = static_cast<uint8_t>((forcePair >> 0) & 0xff);
		forces[destinationIndex + 4] = static_cast<uint8_t>((forcePair >> 8) & 0xff);
		forces[destinationIndex + 5] = 0x00;
		forces[destinationIndex + 6] = 0x00;
		forces[destinationIndex + 7] = 0x00;
		forces[destinationIndex + 8] = 0x00;
		forces[destinationIndex + 9] = 0x00;
		forces[destinationIndex + 10] = 0x00;
		return true;
	}
	return Off(forces, destinationIndex);
}

bool TriggerEffectGenerator::Galloping(uint8_t(&forces)[11], size_t destinationIndex, uint8_t startPosition, uint8_t endPosition, uint8_t firstFoot, uint8_t secondFoot, uint8_t frequency) {
	if (destinationIndex + 10 >= 11) return false;
	if (startPosition > 8 || endPosition > 9 || startPosition >= endPosition || secondFoot > 7 || firstFoot > 6 || firstFoot >= secondFoot) return false;
	if (frequency > 0) {
		uint16_t startAndStopZones = static_cast<uint16_t>((1 << startPosition) | (1 << endPosition));
		uint32_t timeAndRatio = static_cast<uint32_t>(((secondFoot & 0x07) << (3 * 0)) | ((firstFoot & 0x07) << (3 * 1)));
		forces[destinationIndex + 0] = static_cast<uint8_t>(TriggerEffectType::Galloping);
		forces[destinationIndex + 1] = static_cast<uint8_t>((startAndStopZones >> 0) & 0xff);
		forces[destinationIndex + 2] = static_cast<uint8_t>((startAndStopZones >> 8) & 0xff);
		forces[destinationIndex + 3] = static_cast<uint8_t>((timeAndRatio >> 0) & 0xff);
		forces[destinationIndex + 4] = frequency;
		forces[destinationIndex + 5] = 0x00;
		forces[destinationIndex + 6] = 0x00;
		forces[destinationIndex + 7] = 0x00;
		forces[destinationIndex + 8] = 0x00;
		forces[destinationIndex + 9] = 0x00;
		forces[destinationIndex + 10] = 0x00;
		return true;
	}
	return Off(forces, destinationIndex);
}

bool TriggerEffectGenerator::Machine(uint8_t(&forces)[11], size_t destinationIndex, uint8_t startPosition, uint8_t endPosition, uint8_t amplitudeA, uint8_t amplitudeB, uint8_t frequency, uint8_t period) {
	if (destinationIndex + 10 >= 11) return false;
	if (startPosition > 8 || endPosition > 9 || endPosition <= startPosition || amplitudeA > 7 || amplitudeB > 7) return false;
	if (frequency > 0) {
		uint16_t startAndStopZones = static_cast<uint16_t>((1 << startPosition) | (1 << endPosition));
		uint32_t strengthPair = static_cast<uint32_t>(((amplitudeA & 0x07) << (3 * 0)) | ((amplitudeB & 0x07) << (3 * 1)));
		forces[destinationIndex + 0] = static_cast<uint8_t>(TriggerEffectType::Machine);
		forces[destinationIndex + 1] = static_cast<uint8_t>((startAndStopZones >> 0) & 0xff);
		forces[destinationIndex + 2] = static_cast<uint8_t>((startAndStopZones >> 8) & 0xff);
		forces[destinationIndex + 3] = static_cast<uint8_t>((strengthPair >> 0) & 0xff);
		forces[destinationIndex + 4] = frequency;
		forces[destinationIndex + 5] = period;
		forces[destinationIndex + 6] = 0x00;
		forces[destinationIndex + 7] = 0x00;
		forces[destinationIndex + 8] = 0x00;
		forces[destinationIndex + 9] = 0x00;
		forces[destinationIndex + 10] = 0x00;
		return true;
	}
	return Off(forces, destinationIndex);
}

bool TriggerEffectGenerator::Simple_Feedback(uint8_t(&forces)[11], size_t destinationIndex, uint8_t position, uint8_t strength) {
	if (destinationIndex + 10 >= 11) return false;
	forces[destinationIndex + 0] = static_cast<uint8_t>(TriggerEffectType::Simple_Feedback);
	forces[destinationIndex + 1] = position;
	forces[destinationIndex + 2] = strength;
	forces[destinationIndex + 3] = 0x00;
	forces[destinationIndex + 4] = 0x00;
	forces[destinationIndex + 5] = 0x00;
	forces[destinationIndex + 6] = 0x00;
	forces[destinationIndex + 7] = 0x00;
	forces[destinationIndex + 8] = 0x00;
	forces[destinationIndex + 9] = 0x00;
	forces[destinationIndex + 10] = 0x00;
	return true;
}

bool TriggerEffectGenerator::Simple_Weapon(uint8_t(&forces)[11], size_t destinationIndex, uint8_t startPosition, uint8_t endPosition, uint8_t strength) {
	if (destinationIndex + 10 >= 11) return false;
	forces[destinationIndex + 0] = static_cast<uint8_t>(TriggerEffectType::Simple_Weapon);
	forces[destinationIndex + 1] = startPosition;
	forces[destinationIndex + 2] = endPosition;
	forces[destinationIndex + 3] = strength;
	forces[destinationIndex + 4] = 0x00;
	forces[destinationIndex + 5] = 0x00;
	forces[destinationIndex + 6] = 0x00;
	forces[destinationIndex + 7] = 0x00;
	forces[destinationIndex + 8] = 0x00;
	forces[destinationIndex + 9] = 0x00;
	forces[destinationIndex + 10] = 0x00;
	return true;
}

bool TriggerEffectGenerator::Simple_Vibration(uint8_t(&forces)[11], size_t destinationIndex, uint8_t position, uint8_t amplitude, uint8_t frequency) {
	if (destinationIndex + 10 >= 11) return false;
	if (frequency > 0 && amplitude > 0) {
		forces[destinationIndex + 0] = static_cast<uint8_t>(TriggerEffectType::Simple_Vibration);
		forces[destinationIndex + 1] = frequency;
		forces[destinationIndex + 2] = amplitude;
		forces[destinationIndex + 3] = position;
		forces[destinationIndex + 4] = 0x00;
		forces[destinationIndex + 5] = 0x00;
		forces[destinationIndex + 6] = 0x00;
		forces[destinationIndex + 7] = 0x00;
		forces[destinationIndex + 8] = 0x00;
		forces[destinationIndex + 9] = 0x00;
		forces[destinationIndex + 10] = 0x00;
		return true;
	}
	return Off(forces, destinationIndex);
}

bool TriggerEffectGenerator::Limited_Feedback(uint8_t(&forces)[11], size_t destinationIndex, uint8_t position, uint8_t strength) {
	if (destinationIndex + 10 >= 11) return false;
	if (strength > 10) return false;
	if (strength > 0) {
		forces[destinationIndex + 0] = static_cast<uint8_t>(TriggerEffectType::Limited_Feedback);
		forces[destinationIndex + 1] = position;
		forces[destinationIndex + 2] = strength;
		forces[destinationIndex + 3] = 0x00;
		forces[destinationIndex + 4] = 0x00;
		forces[destinationIndex + 5] = 0x00;
		forces[destinationIndex + 6] = 0x00;
		forces[destinationIndex + 7] = 0x00;
		forces[destinationIndex + 8] = 0x00;
		forces[destinationIndex + 9] = 0x00;
		forces[destinationIndex + 10] = 0x00;
		return true;
	}
	return Off(forces, destinationIndex);
}

bool TriggerEffectGenerator::Limited_Weapon(uint8_t(&forces)[11], size_t destinationIndex, uint8_t startPosition, uint8_t endPosition, uint8_t strength) {
	if (destinationIndex + 10 >= 11) return false;
	if (startPosition < 0x10 || endPosition < startPosition || (startPosition + 100) < endPosition || strength > 10) return false;
	if (strength > 0) {
		forces[destinationIndex + 0] = static_cast<uint8_t>(TriggerEffectType::Limited_Weapon);
		forces[destinationIndex + 1] = startPosition;
		forces[destinationIndex + 2] = endPosition;
		forces[destinationIndex + 3] = strength;
		forces[destinationIndex + 4] = 0x00;
		forces[destinationIndex + 5] = 0x00;
		forces[destinationIndex + 6] = 0x00;
		forces[destinationIndex + 7] = 0x00;
		forces[destinationIndex + 8] = 0x00;
		forces[destinationIndex + 9] = 0x00;
		forces[destinationIndex + 10] = 0x00;
		return true;
	}
	return Off(forces, destinationIndex);
}

// Apple class implementations
bool TriggerEffectGenerator::Apple::SetModeOff(uint8_t(&forces)[11], size_t destinationIndex) {
	return TriggerEffectGenerator::Off(forces, destinationIndex);
}

bool TriggerEffectGenerator::Apple::SetModeFeedbackWithStartPosition(uint8_t(&forces)[11], size_t destinationIndex, float startPosition, float resistiveStrength) {
	uint8_t pos = static_cast<uint8_t>(std::round(startPosition * 9.0f));
	uint8_t str = static_cast<uint8_t>(std::round(resistiveStrength * 8.0f));
	return TriggerEffectGenerator::Feedback(forces, destinationIndex, pos, str);
}

bool TriggerEffectGenerator::Apple::SetModeWeaponWithStartPosition(uint8_t(&forces)[11], size_t destinationIndex, float startPosition, float endPosition, float resistiveStrength) {
	uint8_t start = static_cast<uint8_t>(std::round(startPosition * 9.0f));
	uint8_t end = static_cast<uint8_t>(std::round(endPosition * 9.0f));
	uint8_t str = static_cast<uint8_t>(std::round(resistiveStrength * 8.0f));
	return TriggerEffectGenerator::Weapon(forces, destinationIndex, start, end, str);
}

bool TriggerEffectGenerator::Apple::SetModeVibrationWithStartPosition(uint8_t(&forces)[11], size_t destinationIndex, float startPosition, float amplitude, float frequency) {
	uint8_t pos = static_cast<uint8_t>(std::round(startPosition * 9.0f));
	uint8_t amp = static_cast<uint8_t>(std::round(amplitude * 8.0f));
	uint8_t freq = static_cast<uint8_t>(std::round(frequency * 255.0f));
	return TriggerEffectGenerator::Vibration(forces, destinationIndex, pos, amp, freq);
}

bool TriggerEffectGenerator::Apple::SetModeFeedback(uint8_t(&forces)[11], size_t destinationIndex, const std::array<float, 10>& positionalResistiveStrengths) {
	uint8_t force[10];
	for (size_t i = 0; i < 10; i++) {
		force[i] = static_cast<uint8_t>(std::round(positionalResistiveStrengths[i] * 8.0f));
	}
	return TriggerEffectGenerator::MultiplePositionFeedback(forces, destinationIndex, force);
}

bool TriggerEffectGenerator::Apple::SetModeSlopeFeedback(uint8_t(&forces)[11], size_t destinationIndex, float startPosition, float endPosition, float startStrength, float endStrength) {
	uint8_t startPos = static_cast<uint8_t>(std::round(startPosition * 9.0f));
	uint8_t endPos = static_cast<uint8_t>(std::round(endPosition * 9.0f));
	uint8_t startStr = static_cast<uint8_t>(std::round(startStrength * 8.0f));
	uint8_t endStr = static_cast<uint8_t>(std::round(endStrength * 8.0f));
	return TriggerEffectGenerator::SlopeFeedback(forces, destinationIndex, startPos, endPos, startStr, endStr);
}

bool TriggerEffectGenerator::Apple::SetModeVibration(uint8_t(&forces)[11], size_t destinationIndex, const std::array<float, 10>& positionalAmplitudes, float frequency) {
	uint8_t strength[10];
	for (size_t i = 0; i < 10; i++) {
		strength[i] = static_cast<uint8_t>(std::round(positionalAmplitudes[i] * 8.0f));
	}
	uint8_t freq = static_cast<uint8_t>(std::round(frequency * 255.0f));
	return TriggerEffectGenerator::MultiplePositionVibration(forces, destinationIndex, freq, strength);
}

// ReWASD class implementations
bool TriggerEffectGenerator::ReWASD::FullPress(uint8_t(&forces)[11], size_t destinationIndex) {
	return TriggerEffectGenerator::Simple_Weapon(forces, destinationIndex, 0x90, 0xa0, 0xff);
}

bool TriggerEffectGenerator::ReWASD::SoftPress(uint8_t(&forces)[11], size_t destinationIndex) {
	return TriggerEffectGenerator::Simple_Weapon(forces, destinationIndex, 0x70, 0xa0, 0xff);
}

bool TriggerEffectGenerator::ReWASD::MediumPress(uint8_t(&forces)[11], size_t destinationIndex) {
	return TriggerEffectGenerator::Simple_Weapon(forces, destinationIndex, 0x45, 0xa0, 0xff);
}

bool TriggerEffectGenerator::ReWASD::HardPress(uint8_t(&forces)[11], size_t destinationIndex) {
	return TriggerEffectGenerator::Simple_Weapon(forces, destinationIndex, 0x20, 0xa0, 0xff);
}

bool TriggerEffectGenerator::ReWASD::Pulse(uint8_t(&forces)[11], size_t destinationIndex) {
	return TriggerEffectGenerator::Simple_Weapon(forces, destinationIndex, 0x00, 0x00, 0x00);
}

bool TriggerEffectGenerator::ReWASD::Choppy(uint8_t(&forces)[11], size_t destinationIndex) {
	if (destinationIndex + 10 >= 11) return false;
	forces[destinationIndex + 0] = static_cast<uint8_t>(TriggerEffectType::Feedback);
	forces[destinationIndex + 1] = 0x02;
	forces[destinationIndex + 2] = 0x27;
	forces[destinationIndex + 3] = 0x18;
	forces[destinationIndex + 4] = 0x00;
	forces[destinationIndex + 5] = 0x00;
	forces[destinationIndex + 6] = 0x26;
	forces[destinationIndex + 7] = 0x00;
	forces[destinationIndex + 8] = 0x00;
	forces[destinationIndex + 9] = 0x00;
	forces[destinationIndex + 10] = 0x00;
	return true;
}

bool TriggerEffectGenerator::ReWASD::SoftRigidity(uint8_t(&forces)[11], size_t destinationIndex) {
	return TriggerEffectGenerator::Simple_Feedback(forces, destinationIndex, 0x00, 0x00);
}

bool TriggerEffectGenerator::ReWASD::MediumRigidity(uint8_t(&forces)[11], size_t destinationIndex) {
	return TriggerEffectGenerator::Simple_Feedback(forces, destinationIndex, 0x00, 0x64);
}

bool TriggerEffectGenerator::ReWASD::MaxRigidity(uint8_t(&forces)[11], size_t destinationIndex) {
	return TriggerEffectGenerator::Simple_Feedback(forces, destinationIndex, 0x00, 0xdc);
}

bool TriggerEffectGenerator::ReWASD::HalfPress(uint8_t(&forces)[11], size_t destinationIndex) {
	return TriggerEffectGenerator::Simple_Feedback(forces, destinationIndex, 0x55, 0x64);
}

bool TriggerEffectGenerator::ReWASD::Rifle(uint8_t(&forces)[11], size_t destinationIndex, uint8_t frequency) {
	if (destinationIndex + 10 >= 11) return false;
	if (frequency < 2 || frequency > 20) return false;
	forces[destinationIndex + 0] = static_cast<uint8_t>(TriggerEffectType::Vibration);
	forces[destinationIndex + 1] = 0x00;
	forces[destinationIndex + 2] = 0x03;
	forces[destinationIndex + 3] = 0x00;
	forces[destinationIndex + 4] = 0x00;
	forces[destinationIndex + 5] = 0x00;
	forces[destinationIndex + 6] = 0x3F;
	forces[destinationIndex + 7] = 0x00;
	forces[destinationIndex + 8] = 0x00;
	forces[destinationIndex + 9] = frequency;
	forces[destinationIndex + 10] = 0x00;
	return true;
}

bool TriggerEffectGenerator::ReWASD::Vibration(uint8_t(&forces)[11], size_t destinationIndex, uint8_t strength, uint8_t frequency) {
	if (destinationIndex + 10 >= 11) return false;
	if (strength < 1 || frequency < 1) return false;
	forces[destinationIndex + 0] = static_cast<uint8_t>(TriggerEffectType::Vibration);
	forces[destinationIndex + 1] = 0x00;
	forces[destinationIndex + 2] = 0x03;
	forces[destinationIndex + 3] = 0x00;
	forces[destinationIndex + 4] = 0x00;
	forces[destinationIndex + 5] = 0x00;
	forces[destinationIndex + 6] = strength;
	forces[destinationIndex + 7] = 0x00;
	forces[destinationIndex + 8] = 0x00;
	forces[destinationIndex + 9] = frequency;
	forces[destinationIndex + 10] = 0x00;
	return true;
}