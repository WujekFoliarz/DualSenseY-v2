#ifndef DUALIB_DATA_STRUCTURES_H
#define DUALIB_DATA_STRUCTURES_H

// Source https://controllers.fandom.com/wiki/Sony_DualSense
//		  https://controllers.fandom.com/wiki/Sony_DualShock_4

#pragma pack(push, 1)
enum class Direction : uint8_t {
	North,
	NorthEast,
	East,
	SouthEast,
	South,
	SouthWest,
	West,
	NorthWest,
	None
};

template<int N> struct BTCRC {
	uint8_t Buff[N - 4];
	uint32_t CRC;
};
#pragma pack(pop)

#pragma pack(push, 1)
namespace dualsenseData {
	struct TouchFingerData {
		uint32_t Index : 7;
		uint32_t NotTouching : 1;
		uint32_t FingerX : 12;
		uint32_t FingerY : 12;
	};

	struct TouchData {
		TouchFingerData Finger[2];
		uint8_t Timestamp;
	};

	enum class PowerState : uint8_t {
		Discharging = 0x00, // Use PowerPercent
		Charging = 0x01, // Use PowerPercent
		Complete = 0x02, // PowerPercent not valid? assume 100%?
		AbnormalVoltage = 0x0A, // PowerPercent not valid?
		AbnormalTemperature = 0x0B, // PowerPercent not valid?
		ChargingError = 0x0F  // PowerPercent not valid?
	};

	enum class MuteLight : uint8_t {
		Off,
		On,
		Breathing,
		DoNothing, // literally nothing, this input is ignored,
		// though it might be a faster blink in other versions
		NoAction4,
		NoAction5,
		NoAction6,
		NoAction7
	};

	enum class LightBrightness : uint8_t {
		Bright,
		Mid,
		Dim,
		NoAction3,
		NoAction4,
		NoAction5,
		NoAction6,
		NoAction7
	};

	enum class LightFadeAnimation : uint8_t {
		Nothing,
		FadeIn, // from black to blue
		FadeOut // from blue to black
	};

	struct ReportFeatureInMacAll {
		uint8_t ReportID; // 0x09
		uint8_t ClientMac[6]; // Right to Left
		uint8_t Hard08;
		uint8_t Hard25;
		uint8_t Hard00;
		uint8_t HostMac[6]; // Right to Left
		uint8_t Pad[3]; // Size according to Linux driver
	};

	struct ReportFeatureInVersion {
		union {
			BTCRC<64> CRC;
			struct {
				uint8_t ReportID; // 0x20
				char BuildDate[11]; // string
				char BuildTime[8]; // string
				uint16_t FwType;
				uint16_t SwSeries;
				uint32_t HardwareInfo; // 0x00FF0000 - Variation
				// 0x0000FF00 - Generation
				// 0x0000003F - Trial?
				// ^ Values tied to enumerations
				uint32_t FirmwareVersion; // 0xAABBCCCC AA.BB.CCCC
				char DeviceInfo[12];
				////
				uint16_t UpdateVersion;
				char UpdateImageInfo;
				char UpdateUnk;
				////
				uint32_t FwVersion1; // AKA SblFwVersion
				// 0xAABBCCCC AA.BB.CCCC
				// Ignored for FwType 0
				// HardwareVersion used for FwType 1
				// Unknown behavior if HardwareVersion < 0.1.38 for FwType 2 & 3
				// If HardwareVersion >= 0.1.38 for FwType 2 & 3
				uint32_t FwVersion2; // AKA VenomFwVersion
				uint32_t FwVersion3; // AKA SpiderDspFwVersion AKA BettyFwVer
				// May be Memory Control Unit for Non Volatile Storage
			};
		};
	};

	struct BTSimpleGetStateData { // 9
		/*0  */ uint8_t LeftStickX;
		/*1  */ uint8_t LeftStickY;
		/*2  */ uint8_t RightStickX;
		/*3  */ uint8_t RightStickY;
		/*4.0*/ Direction DPad : 4;
		/*4.4*/ uint8_t ButtonSquare : 1;
		/*4.5*/ uint8_t ButtonCross : 1;
		/*4.6*/ uint8_t ButtonCircle : 1;
		/*4.7*/ uint8_t ButtonTriangle : 1;
		/*5.0*/ uint8_t ButtonL1 : 1;
		/*5.1*/ uint8_t ButtonR1 : 1;
		/*5.2*/ uint8_t ButtonL2 : 1;
		/*5.3*/ uint8_t ButtonR2 : 1;
		/*5.4*/ uint8_t ButtonShare : 1;
		/*5.5*/ uint8_t ButtonOptions : 1;
		/*5.6*/ uint8_t ButtonL3 : 1;
		/*5.7*/ uint8_t ButtonR3 : 1;
		/*6.1*/ uint8_t ButtonHome : 1;
		/*6.2*/ uint8_t ButtonPad : 1;
		/*6.3*/ uint8_t Counter : 6;
		/*7  */ uint8_t TriggerLeft;
		/*8  */ uint8_t TriggerRight;
		// anything beyond this point, if set, is invalid junk data that was not cleared
	};

	struct USBGetStateData { // 63
		/* 0  */ uint8_t LeftStickX;
		/* 1  */ uint8_t LeftStickY;
		/* 2  */ uint8_t RightStickX;
		/* 3  */ uint8_t RightStickY;
		/* 4  */ uint8_t TriggerLeft;
		/* 5  */ uint8_t TriggerRight;
		/* 6  */ uint8_t SeqNo; // always 0x01 on BT
		/* 7.0*/ Direction DPad : 4;
		/* 7.4*/ uint8_t ButtonSquare : 1;
		/* 7.5*/ uint8_t ButtonCross : 1;
		/* 7.6*/ uint8_t ButtonCircle : 1;
		/* 7.7*/ uint8_t ButtonTriangle : 1;
		/* 8.0*/ uint8_t ButtonL1 : 1;
		/* 8.1*/ uint8_t ButtonR1 : 1;
		/* 8.2*/ uint8_t ButtonL2 : 1;
		/* 8.3*/ uint8_t ButtonR2 : 1;
		/* 8.4*/ uint8_t ButtonCreate : 1;
		/* 8.5*/ uint8_t ButtonOptions : 1;
		/* 8.6*/ uint8_t ButtonL3 : 1;
		/* 8.7*/ uint8_t ButtonR3 : 1;
		/* 9.0*/ uint8_t ButtonHome : 1;
		/* 9.1*/ uint8_t ButtonPad : 1;
		/* 9.2*/ uint8_t ButtonMute : 1;
		/* 9.3*/ uint8_t UNK1 : 1; // appears unused
		/* 9.4*/ uint8_t ButtonLeftFunction : 1; // DualSense Edge
		/* 9.5*/ uint8_t ButtonRightFunction : 1; // DualSense Edge
		/* 9.6*/ uint8_t ButtonLeftPaddle : 1; // DualSense Edge
		/* 9.7*/ uint8_t ButtonRightPaddle : 1; // DualSense Edge
		/*10  */ uint8_t UNK2; // appears unused
		/*11  */ uint32_t UNK_COUNTER; // Linux driver calls this reserved, tools leak calls the 2 high bytes "random"
		/*15  */ int16_t AngularVelocityX;
		/*17  */ int16_t AngularVelocityZ;
		/*19  */ int16_t AngularVelocityY;
		/*21  */ int16_t AccelerometerX;
		/*23  */ int16_t AccelerometerY;
		/*25  */ int16_t AccelerometerZ;
		/*27  */ uint32_t SensorTimestamp;
		/*31  */ int8_t Temperature; // reserved2 in Linux driver
		/*32  */ TouchData touchData;
		/*41.0*/ uint8_t TriggerRightStopLocation : 4; // trigger stop can be a range from 0 to 9 (F/9.0 for Apple interface)
		/*41.4*/ uint8_t TriggerRightStatus : 4;
		/*42.0*/ uint8_t TriggerLeftStopLocation : 4;
		/*42.4*/ uint8_t TriggerLeftStatus : 4; // 0 feedbackNoLoad
		// 1 feedbackLoadApplied
		// 0 weaponReady
		// 1 weaponFiring
		// 2 weaponFired
		// 0 vibrationNotVibrating
		// 1 vibrationIsVibrating
		/*43  */ uint32_t HostTimestamp; // mirrors data from report write
		/*47.0*/ uint8_t TriggerRightEffect : 4; // Active trigger effect, previously we thought this was status max
		/*47.4*/ uint8_t TriggerLeftEffect : 4;  // 0 for reset and all other effects
		// 1 for feedback effect
		// 2 for weapon effect
		// 3 for vibration
		/*48  */ uint32_t DeviceTimeStamp;
		/*52.0*/ uint8_t PowerPercent : 4; // 0x00-0x0A
		/*52.4*/ PowerState powerState : 4;
		/*53.0*/ uint8_t PluggedHeadphones : 1;
		/*53.1*/ uint8_t PluggedMic : 1;
		/*53.2*/ uint8_t MicMuted : 1; // Mic muted by powersave/mute command
		/*53.3*/ uint8_t PluggedUsbData : 1;
		/*53.4*/ uint8_t PluggedUsbPower : 1;
		/*53.5*/ uint8_t PluggedUnk1 : 3;
		/*54.0*/ uint8_t PluggedExternalMic : 1; // Is external mic active (automatic in mic auto mode)
		/*54.1*/ uint8_t HapticLowPassFilter : 1; // Is the Haptic Low-Pass-Filter active?
		/*54.2*/ uint8_t PluggedUnk3 : 6;
		/*55  */ uint8_t AesCmac[8];
	};

	struct BTGetStateData { // 77
		/* 0*/ USBGetStateData StateData;
		/*63*/ uint8_t UNK1; // Oscillates between 00101100 and 00101101 when rumbling
		// Not affected by rumble volume or enhanced vs normal rumble
		// Audio rumble not yet tested as this is only on BT
		/*64*/ uint8_t BtCrcFailCount;
		/*65*/ uint8_t Pad[11];
	};

	struct ReportIn01USB {
		uint8_t ReportID = 0x01;
		USBGetStateData State;
	};

	struct ReportIn01BT {
		uint8_t ReportID = 0x01;
		USBGetStateData State;
	};

	struct SetStateData { // 47
		/*    */ // Report Set Flags
		/*    */ // These flags are used to indicate what contents from this report should be processed
		/* 0.0*/ uint8_t EnableRumbleEmulation : 1; // Suggest halving rumble strength
		/* 0.1*/ uint8_t UseRumbleNotHaptics : 1; // 
		/*    */
		/* 0.2*/ uint8_t AllowRightTriggerFFB : 1; // Enable setting RightTriggerFFB
		/* 0.3*/ uint8_t AllowLeftTriggerFFB : 1;  // Enable setting LeftTriggerFFB
		/*    */
		/* 0.4*/ uint8_t AllowHeadphoneVolume : 1; // Enable setting VolumeHeadphones
		/* 0.5*/ uint8_t AllowSpeakerVolume : 1;   // Enable setting VolumeSpeaker
		/* 0.6*/ uint8_t AllowMicVolume : 1;       // Enable setting VolumeMic
		/*    */
		/* 0.7*/ uint8_t AllowAudioControl : 1; // Enable setting AudioControl section
		/* 1.0*/ uint8_t AllowMuteLight : 1;    // Enable setting MuteLightMode
		/* 1.1*/ uint8_t AllowAudioMute : 1;    // Enable setting MuteControl section
		/*    */
		/* 1.2*/ uint8_t AllowLedColor : 1; // Enable RGB LED section
		/*    */
		/* 1.3*/ uint8_t ResetLights : 1; // Release the LEDs from Wireless firmware control
		/*    */                         // When in wireless mode this must be signaled to control LEDs
		/*    */                         // This cannot be applied during the BT pair animation.
		/*    */                         // SDL2 waits until the SensorTimestamp value is >= 10200000
		/*    */                         // before pulsing this bit once.
		/*    */
		/* 1.4*/ uint8_t AllowPlayerIndicators : 1; // Enable setting PlayerIndicators section
		/* 1.5*/ uint8_t AllowHapticLowPassFilter : 1; // Enable HapticLowPassFilter
		/* 1.6*/ uint8_t AllowMotorPowerLevel : 1; // MotorPowerLevel reductions for trigger/haptic
		/* 1.7*/ uint8_t AllowAudioControl2 : 1; // Enable setting AudioControl2 section
		/*    */
		/* 2  */ uint8_t RumbleEmulationRight; // emulates the light weight
		/* 3  */ uint8_t RumbleEmulationLeft; // emulated the heavy weight
		/*    */
		/* 4  */ uint8_t VolumeHeadphones; // max 0x7f
		/* 5  */ uint8_t VolumeSpeaker; // PS5 appears to only use the range 0x3d-0x64
		/* 6  */ uint8_t VolumeMic; // not linier, seems to max at 64, 0 is not fully muted
		/*    */
		/*    */ // AudioControl
		/* 7.0*/ uint8_t MicSelect : 2; // 0 Auto
		/*    */                       // 1 Internal Only
		/*    */                       // 2 External Only
		/*    */                       // 3 Unclear, sets external mic flag but might use internal mic, do test
		/* 7.2*/ uint8_t EchoCancelEnable : 1;
		/* 7.3*/ uint8_t NoiseCancelEnable : 1;
		/* 7.4*/ uint8_t OutputPathSelect : 2; // 0 L_R_X
		/*    */                              // 1 L_L_X
		/*    */                              // 2 L_L_R
		/*    */                              // 3 X_X_R
		/* 7.6*/ uint8_t InputPathSelect : 2;  // 0 CHAT_ASR
		/*    */                              // 1 CHAT_CHAT
		/*    */                              // 2 ASR_ASR
		/*    */                              // 3 Does Nothing, invalid
		/*    */
		/* 8  */ MuteLight MuteLightMode;
		/*    */
		/*    */ // MuteControl
		/* 9.0*/ uint8_t TouchPowerSave : 1;
		/* 9.1*/ uint8_t MotionPowerSave : 1;
		/* 9.2*/ uint8_t HapticPowerSave : 1; // AKA BulletPowerSave
		/* 9.3*/ uint8_t AudioPowerSave : 1;
		/* 9.4*/ uint8_t MicMute : 1;
		/* 9.5*/ uint8_t SpeakerMute : 1;
		/* 9.6*/ uint8_t HeadphoneMute : 1;
		/* 9.7*/ uint8_t HapticMute : 1; // AKA BulletMute
		/*    */
		/*10  */ uint8_t RightTriggerFFB[11];
		/*21  */ uint8_t LeftTriggerFFB[11];
		/*32  */ uint32_t HostTimestamp; // mirrored into report read
		/*    */
		/*    */ // MotorPowerLevel
		/*36.0*/ uint8_t TriggerMotorPowerReduction : 4; // 0x0-0x7 (no 0x8?) Applied in 12.5% reductions
		/*36.4*/ uint8_t RumbleMotorPowerReduction : 4;  // 0x0-0x7 (no 0x8?) Applied in 12.5% reductions
		/*    */
		/*    */ // AudioControl2
		/*37.0*/ uint8_t SpeakerCompPreGain : 3; // additional speaker volume boost
		/*37.3*/ uint8_t BeamformingEnable : 1; // Probably for MIC given there's 2, might be more bits, can't find what it does
		/*37.4*/ uint8_t UnkAudioControl2 : 4; // some of these bits might apply to the above
		/*    */
		/*38.0*/ uint8_t AllowLightBrightnessChange : 1; // LED_BRIHTNESS_CONTROL
		/*38.1*/ uint8_t AllowColorLightFadeAnimation : 1; // LIGHTBAR_SETUP_CONTROL
		/*38.2*/ uint8_t EnableImprovedRumbleEmulation : 1; // Use instead of EnableRumbleEmulation
		// requires FW >= 0x0224
		// No need to halve rumble strength
		/*38.3*/ uint8_t UNKBITC : 5; // unused
		/*    */
		/*39.0*/ uint8_t HapticLowPassFilter : 1;
		/*39.1*/ uint8_t UNKBIT : 7;
		/*    */
		/*40  */ uint8_t UNKBYTE; // previous notes suggested this was HLPF, was probably off by 1
		/*    */
		/*41  */ LightFadeAnimation lightFadeAnimation;
		/*42  */ LightBrightness lightBrightness;
		/*    */
		/*    */ // PlayerIndicators
		/*    */ // These bits control the white LEDs under the touch pad.
		/*    */ // Note the reduction in functionality for later revisions.
		/*    */ // Generation 0x03 - Full Functionality
		/*    */ // Generation 0x04 - Mirrored Only
		/*    */ // Suggested detection: (HardwareInfo & 0x00FFFF00) == 0X00000400
		/*    */ //
		/*    */ // Layout used by PS5:
		/*    */ // 0x04 - -x- -  Player 1
		/*    */ // 0x06 - x-x -  Player 2
		/*    */ // 0x15 x -x- x  Player 3
		/*    */ // 0x1B x x-x x  Player 4
		/*    */ // 0x1F x xxx x  Player 5* (Unconfirmed)
		/*    */ //
		/*    */ //                        // HW 0x03 // HW 0x04
		/*43.0*/ uint8_t PlayerLight1 : 1; // x --- - // x --- x
		/*43.1*/ uint8_t PlayerLight2 : 1; // - x-- - // - x-x -
		/*43.2*/ uint8_t PlayerLight3 : 1; // - -x- - // - -x- -
		/*43.3*/ uint8_t PlayerLight4 : 1; // - --x - // - x-x -
		/*43.4*/ uint8_t PlayerLight5 : 1; // - --- x // x --- x
		/*43.5*/ uint8_t PlayerLightFade : 1; // if low player lights fade in, if high player lights instantly change
		/*43.6*/ uint8_t PlayerLightUNK : 2;
		/*    */
		/*    */ // RGB LED
		/*44  */ uint8_t LedRed;
		/*45  */ uint8_t LedGreen;
		/*46  */ uint8_t LedBlue;
		// Structure ends here though on BT there is padding and a CRC, see ReportOut31

		bool operator==(const SetStateData& other) const {
			return
				std::memcmp(RightTriggerFFB, other.RightTriggerFFB, 11) == 0 &&
				std::memcmp(LeftTriggerFFB, other.LeftTriggerFFB, 11) == 0 &&
				LedRed == other.LedRed &&
				LedGreen == other.LedGreen &&
				LedBlue == other.LedBlue &&
				PlayerLight1 == other.PlayerLight1 &&
				PlayerLight2 == other.PlayerLight2 &&
				PlayerLight3 == other.PlayerLight3 &&
				PlayerLight4 == other.PlayerLight4 &&
				PlayerLight5 == other.PlayerLight5 &&
				MuteLightMode == other.MuteLightMode &&
				RumbleEmulationLeft == other.RumbleEmulationLeft &&
				RumbleEmulationRight == other.RumbleEmulationRight &&
				VolumeSpeaker == other.VolumeSpeaker &&
				VolumeMic == other.VolumeMic &&
				VolumeHeadphones == other.VolumeHeadphones &&
				lightBrightness == other.lightBrightness &&
				OutputPathSelect == other.OutputPathSelect;
		}

		bool operator!=(const SetStateData& other) const {
			return !(*this == other);
		}
	};

	struct ReportOut02 {
		uint8_t ReportID; // 0x02
		SetStateData State;
	};

	struct ReportOut31 {
		union {
			BTCRC<78> CRC;
			struct {
				uint8_t ReportID; // 0x31
				uint8_t flag;
				SetStateData State;
			} Data;
		};
	};

	struct ReportIn31 {
		union {
			BTCRC<78> CRC;
			struct {
				uint8_t ReportID; // 0x31
				uint8_t HasHID : 1; // Present for packets with state data
				uint8_t HasMic : 1; // Looks mutually exclusive, possible mic data
				uint8_t Unk1 : 2;
				uint8_t SeqNo : 4; // unclear progression
				BTGetStateData State;
			} Data;
		};
	};
#pragma pack(pop)
}

namespace dualshock4Data {
#pragma pack(push, 1)


	template<int N> struct BTAudio {
		uint16_t FrameNumber;
		uint8_t AudioTarget; // 0x02 speaker?, 0x24 headset?, 0x03 mic?
		uint8_t SBCData[N - 3];
	};

	struct BasicGetStateData {
		uint8_t LeftStickX;
		uint8_t LeftStickY;
		uint8_t RightStickX;
		uint8_t RightStickY;
		Direction DPad : 4;
		uint8_t ButtonSquare : 1;
		uint8_t ButtonCross : 1;
		uint8_t ButtonCircle : 1;
		uint8_t ButtonTriangle : 1;
		uint8_t ButtonL1 : 1;
		uint8_t ButtonR1 : 1;
		uint8_t ButtonL2 : 1;
		uint8_t ButtonR2 : 1;
		uint8_t ButtonShare : 1;
		uint8_t ButtonOptions : 1;
		uint8_t ButtonL3 : 1;
		uint8_t ButtonR3 : 1;
		uint8_t ButtonHome : 1;
		uint8_t ButtonPad : 1;
		uint8_t Counter : 6; // always 0 on USB, counts up with some skips on BT
		uint8_t TriggerLeft;
		uint8_t TriggerRight;
	};

	struct GetStateData : BasicGetStateData {
		uint16_t Timestamp; // in 5.33us units?
		uint8_t Temperture;
		int16_t AngularVelocityX;
		int16_t AngularVelocityZ;
		int16_t AngularVelocityY;
		int16_t AccelerometerX;
		int16_t AccelerometerY;
		int16_t AccelerometerZ;
		uint8_t ExtData[5]; // range can be set by EXT device
		uint8_t PowerPercent : 4; // 0x00-0x0A or 0x01-0x0B if plugged int
		uint8_t PluggedPowerCable : 1;
		uint8_t PluggedHeadphones : 1;
		uint8_t PluggedMic : 1;
		uint8_t PluggedExt : 1;
		uint8_t unk2[2];
		uint8_t TouchPackets;
		uint8_t PacketCount;
		uint32_t Finger1ID : 7;
		uint32_t Finger1Active : 1;
		uint32_t Finger1X : 12;
		uint32_t Finger1Y : 12;
		uint32_t Finger2ID : 7;
		uint32_t Finger2Active : 1;
		uint32_t Finger2X : 12;
		uint32_t Finger2Y : 12;
	};

	struct USBGetStateData : GetStateData {

	};

	struct BTGetStateData : GetStateData {

	};

	struct ReportIn01USB {
		uint8_t ReportID; // 0x01
		USBGetStateData State = {};
	};

	struct ReportIn01BT {
		uint8_t ReportID; // 0x01
		uint8_t padding : 8;
		uint8_t padding2 : 4;
		USBGetStateData State = {};
	};

	struct ReportFeature {
		uint8_t flag1 : 4;
		uint8_t flag2 : 1;
		uint8_t flag3 : 1;
		uint8_t flag4 : 2;
		uint8_t reportId;
		uint16_t buff;
	};

	struct BTSetStateData {
		uint8_t EnableRumbleUpdate : 1;
		uint8_t EnableLedUpdate : 1;
		uint8_t EnableLedBlink : 1;
		uint8_t EnableExtWrite : 1;
		uint8_t EnableVolumeLeftUpdate : 1;
		uint8_t EnableVolumeRightUpdate : 1;
		uint8_t EnableVolumeMicUpdate : 1;
		uint8_t EnableVolumeSpeakerUpdate : 1;
		uint8_t UNK_RESET1 : 1; // unknown reset, both set high by Remote Play
		uint8_t UNK_RESET2 : 1; // unknown reset, both set high by Remote Play
		uint8_t UNK1 : 1;
		uint8_t UNK2 : 1;
		uint8_t UNK3 : 1;
		uint8_t UNKPad : 3;
		uint8_t Empty1;
		uint8_t RumbleRight; // weak
		uint8_t RumbleLeft; // strong
		uint8_t LedRed;
		uint8_t LedGreen;
		uint8_t LedBlue;
		uint8_t LedFlashOnPeriod;
		uint8_t LedFlashOffPeriod;
		uint8_t ExtDataSend[8]; // sent to I2C EXT port, stored in 8x8 byte block
		uint8_t VolumeLeft; // 0x00 - 0x4F inclusive
		uint8_t VolumeRight; // 0x00 - 0x4F inclusive
		uint8_t VolumeMic; // 0x00, 0x01 - 0x40 inclusive (0x00 is special behavior)
		uint8_t VolumeSpeaker; // 0x00 - 0x4F
		uint8_t UNK_AUDIO1 : 7; // clamped to 1-64 inclusive, appears to be set to 5 for audio
		uint8_t UNK_AUDIO2 : 1; // unknown, appears to be set to 1 for audio
		uint8_t Pad[52];

		bool operator==(const BTSetStateData& other) const {
			return
				LedRed == other.LedRed &&
				LedGreen == other.LedGreen &&
				LedBlue == other.LedBlue &&
				RumbleLeft == other.RumbleLeft &&
				RumbleRight == other.RumbleRight &&
				VolumeSpeaker == other.VolumeSpeaker &&
				VolumeMic == other.VolumeMic &&
				VolumeLeft == other.VolumeLeft &&
				VolumeRight == other.VolumeRight;
		}

		bool operator!=(const BTSetStateData& other) const {
			return !(*this == other);
		}
	};

	template<int N> struct BTSetStateDataAndAudio {
		BTSetStateData State;
		BTAudio<N - 75> Audio;
	};

	struct ReportIn05 {
		uint8_t ReportID; // 0x05
		BTSetStateData State;
	};

	struct ReportIn11 {
		uint8_t flag1;
		uint8_t flag2;
		uint8_t flag3;
		uint8_t ReportID; // 0x11
		BTSetStateData State;
	};

	struct ReportOut11 {
		union {
			BTCRC<78> CRC;
			struct {
				uint8_t ReportID; // 0x11
				uint8_t PollingRate : 6; // note 0 appears to be clamped to 1
				uint8_t EnableCRC : 1;
				uint8_t EnableHID : 1;
				uint8_t EnableMic : 3; // somehow enables mic, appears to be 3 bit flags
				uint8_t UnkA4 : 1;
				uint8_t AllowRed : 1;
				uint8_t AllowBlue : 1; // seems to always be 1
				uint8_t AllowGreen : 1;
				uint8_t EnableAudio : 1;
				union {
					BTSetStateData State;
				};
			} Data;
		};
	};

	struct ReportOut15 {
		union {
			BTCRC<334> CRC;
			struct {
				uint8_t ReportID; // 0x15
				uint8_t PollingRate : 6; // note 0 appears to be clamped to 1
				uint8_t EnableCRC : 1;
				uint8_t EnableHID : 1;
				uint8_t EnableMic : 3; // somehow enables mic, appears to be 3 bit flags
				uint8_t UnkA4 : 1;
				uint8_t UnkB1 : 1;
				uint8_t UnkB2 : 1; // seems to always be 1
				uint8_t UnkB3 : 1;
				uint8_t EnableAudio : 1;
				union {
					BTSetStateDataAndAudio<331> State;
					BTAudio<331> Audio;
				};
			} Data;
		};
	};

	struct ReportFeatureInMacAll {
		uint8_t ReportID; // 0x12 (0x09 for BT)
		uint8_t ClientMac[6]; // Right to Left
		uint8_t Hard08;
		uint8_t Hard25;
		uint8_t Hard00;
		uint8_t HostMac[6]; // Right to Left
	};

	struct ReportFeatureInMacAllBT {
		union {
			BTCRC<53> CRC;
			ReportFeatureInMacAll Data; // with ReportID 0x09
		};
	};

	enum AudioOutput : uint8_t {
		HeadsetStereo = 0,  // Left and Right to headphones
		HeadsetMono,        // Left to headphones
		HeadsetMonoSpeaker, // Left to headphones, Right to speaker
		Speaker,            // Right to speaker
		Disabled = 4
	};
	struct ReportFeatureInDongleSetAudio {
		uint8_t ReportID; // 0xE0
		uint8_t Unknown; // 0x00
		AudioOutput Output;
	};
#pragma pack(pop)
}

#endif // DUALIB_DATA_STRUCTURES_H