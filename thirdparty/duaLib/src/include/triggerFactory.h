#ifndef TRIGGER_EFFECT_GENERATOR_H
#define TRIGGER_EFFECT_GENERATOR_H

#include <array>
#include <cstdint>
#include <cstddef>

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

 /**
  * @brief Enum representing actual effect byte values sent to the DualSense controller.
  * More complex effects may be built through the combination of these values and specific parameters.
  */
enum class TriggerEffectType : uint8_t {
	// Officially recognized modes
	Off = 0x05, // 00 00 0 101
	Feedback = 0x21, // 00 10 0 001
	Weapon = 0x25, // 00 10 0 101
	Vibration = 0x26, // 00 10 0 110

	// Unofficial but unique effects left in the firmware
	Bow = 0x22, // 00 10 0 010
	Galloping = 0x23, // 00 10 0 011
	Machine = 0x27, // 00 10 0 111

	// Leftover versions of official modes with simpler logic and no parameter protections
	Simple_Feedback = 0x01, // 00 00 0 001
	Simple_Weapon = 0x02, // 00 00 0 010
	Simple_Vibration = 0x06, // 00 00 0 110

	// Leftover versions of official modes with limited parameter ranges
	Limited_Feedback = 0x11, // 00 01 0 001
	Limited_Weapon = 0x12, // 00 01 0 010

	// Debug or Calibration functions
	DebugFC = 0xFC, // 11 11 1 100
	DebugFD = 0xFD, // 11 11 1 101
	DebugFE = 0xFE, // 11 11 1 110
};

/**
 * @brief DualSense controller trigger effect generators.
 * Revision: 6
 *
 * If converting from official Sony code, map your effect enum to the corresponding factory
 * function and your parameter struct to the function parameters. Note that you must track
 * the controller's current effect state yourself. All effect factories return false and
 * do not modify the forces array if invalid parameters are used. If parameters result
 * in zero effect, the Off effect is applied, consistent with Sony's official behavior.
 */
class TriggerEffectGenerator {
public:
	// Official Effects
	static bool Off(uint8_t(&forces)[11], size_t destinationIndex);
	static bool Feedback(uint8_t(&forces)[11], size_t destinationIndex, uint8_t position, uint8_t strength);
	static bool Weapon(uint8_t(&forces)[11], size_t destinationIndex, uint8_t startPosition, uint8_t endPosition, uint8_t strength);
	static bool Vibration(uint8_t(&forces)[11], size_t destinationIndex, uint8_t position, uint8_t amplitude, uint8_t frequency);
	static bool MultiplePositionFeedback(uint8_t(&forces)[11], size_t destinationIndex, uint8_t(&strength)[10]);
	static bool SlopeFeedback(uint8_t(&forces)[11], size_t destinationIndex, uint8_t startPosition, uint8_t endPosition, uint8_t startStrength, uint8_t endStrength);
	static bool MultiplePositionVibration(uint8_t(&forces)[11], size_t destinationIndex, uint8_t frequency, uint8_t(&amplitude)[10]);

	// Unofficial but Unique Effects
	static bool Bow(uint8_t(&forces)[11], size_t destinationIndex, uint8_t startPosition, uint8_t endPosition, uint8_t strength, uint8_t snapForce);
	static bool Galloping(uint8_t(&forces)[11], size_t destinationIndex, uint8_t startPosition, uint8_t endPosition, uint8_t firstFoot, uint8_t secondFoot, uint8_t frequency);
	static bool Machine(uint8_t(&forces)[11], size_t destinationIndex, uint8_t startPosition, uint8_t endPosition, uint8_t amplitudeA, uint8_t amplitudeB, uint8_t frequency, uint8_t period);

	// Simple Effects
	static bool Simple_Feedback(uint8_t(&forces)[11], size_t destinationIndex, uint8_t position, uint8_t strength);
	static bool Simple_Weapon(uint8_t(&forces)[11], size_t destinationIndex, uint8_t startPosition, uint8_t endPosition, uint8_t strength);
	static bool Simple_Vibration(uint8_t(&forces)[11], size_t destinationIndex, uint8_t position, uint8_t amplitude, uint8_t frequency);

	// Limited Effects
	static bool Limited_Feedback(uint8_t(&forces)[11], size_t destinationIndex, uint8_t position, uint8_t strength);
	static bool Limited_Weapon(uint8_t(&forces)[11], size_t destinationIndex, uint8_t startPosition, uint8_t endPosition, uint8_t strength);

	/**
	 * @brief Interface adapters patterned after Apple's GCDualSenseAdaptiveTrigger class.
	 */
	class Apple {
	public:
		static bool SetModeOff(uint8_t(&forces)[11], size_t destinationIndex);
		static bool SetModeFeedbackWithStartPosition(uint8_t(&forces)[11], size_t destinationIndex, float startPosition, float resistiveStrength);
		static bool SetModeWeaponWithStartPosition(uint8_t(&forces)[11], size_t destinationIndex, float startPosition, float endPosition, float resistiveStrength);
		static bool SetModeVibrationWithStartPosition(uint8_t(&forces)[11], size_t destinationIndex, float startPosition, float amplitude, float frequency);
		static bool SetModeFeedback(uint8_t(&forces)[11], size_t destinationIndex, const std::array<float, 10>& positionalResistiveStrengths);
		static bool SetModeSlopeFeedback(uint8_t(&forces)[11], size_t destinationIndex, float startPosition, float endPosition, float startStrength, float endStrength);
		static bool SetModeVibration(uint8_t(&forces)[11], size_t destinationIndex, const std::array<float, 10>& positionalAmplitudes, float frequency);
	};

	/**
	 * @brief Interface adapters patterned after reWASD's actual interface.
	 */
	class ReWASD {
	public:
		static bool FullPress(uint8_t(&forces)[11], size_t destinationIndex);
		static bool SoftPress(uint8_t(&forces)[11], size_t destinationIndex);
		static bool MediumPress(uint8_t(&forces)[11], size_t destinationIndex);
		static bool HardPress(uint8_t(&forces)[11], size_t destinationIndex);
		static bool Pulse(uint8_t(&forces)[11], size_t destinationIndex);
		static bool Choppy(uint8_t(&forces)[11], size_t destinationIndex);
		static bool SoftRigidity(uint8_t(&forces)[11], size_t destinationIndex);
		static bool MediumRigidity(uint8_t(&forces)[11], size_t destinationIndex);
		static bool MaxRigidity(uint8_t(&forces)[11], size_t destinationIndex);
		static bool HalfPress(uint8_t(&forces)[11], size_t destinationIndex);
		static bool Rifle(uint8_t(&forces)[11], size_t destinationIndex, uint8_t frequency = 10);
		static bool Vibration(uint8_t(&forces)[11], size_t destinationIndex, uint8_t strength = 220, uint8_t frequency = 30);
	};
};

#endif // TRIGGER_EFFECT_GENERATOR_H