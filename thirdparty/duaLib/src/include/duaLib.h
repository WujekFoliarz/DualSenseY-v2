#ifndef DUALIB_H
#define DUALIB_H

#include <stdint.h>
#include <string>

#ifndef _SCE_PAD_TRIGGER_EFFECT_H
#define _SCE_PAD_TRIGGER_EFFECT_H

#define SCE_PAD_TRIGGER_EFFECT_TRIGGER_MASK_L2			0x01
#define SCE_PAD_TRIGGER_EFFECT_TRIGGER_MASK_R2			0x02
#define SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2		0
#define SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_R2		1
#define SCE_PAD_TRIGGER_EFFECT_TRIGGER_NUM				2
#define SCE_PAD_TRIGGER_EFFECT_CONTROL_POINT_NUM		10

#define SCE_PAD_TRIGGER_STATE_FEEDBACK_NO_FORCE 1
#define SCE_PAD_TRIGGER_STATE_FEEDBACK_IS_PUSHING 2
#define SCE_PAD_TRIGGER_STATE_WEAPON_NOT_PRESSED 3
#define SCE_PAD_TRIGGER_STATE_WEAPON_ALMOST_PRESSED 4
#define SCE_PAD_TRIGGER_STATE_WEAPON_FULLY_PRESSED 5
#define SCE_PAD_TRIGGER_STATE_VIBRATION_NOT_FIRING 6
#define SCE_PAD_TRIGGER_STATE_VIBRATION_IS_FIRING 7

typedef enum ScePadTriggerEffectMode {
	SCE_PAD_TRIGGER_EFFECT_MODE_OFF,
	SCE_PAD_TRIGGER_EFFECT_MODE_FEEDBACK,
	SCE_PAD_TRIGGER_EFFECT_MODE_WEAPON,
	SCE_PAD_TRIGGER_EFFECT_MODE_VIBRATION,
	SCE_PAD_TRIGGER_EFFECT_MODE_MULTIPLE_POSITION_FEEDBACK,
	SCE_PAD_TRIGGER_EFFECT_MODE_SLOPE_FEEDBACK,
	SCE_PAD_TRIGGER_EFFECT_MODE_MULTIPLE_POSITION_VIBRATION,
} ScePadTriggerEffectMode;

/**
 *E
 *  @brief parameter for setting the trigger effect to off mode.
 *         Off Mode: Stop trigger effect.
 **/
typedef struct ScePadTriggerEffectOffParam {
	uint8_t padding[48];
} ScePadTriggerEffectOffParam;

/**
 *E
 *  @brief parameter for setting the trigger effect to Feedback mode.
 *         Feedback Mode: The motor arm pushes back trigger.
 *                        Trigger obtains stiffness at specified position.
 **/
typedef struct ScePadTriggerEffectFeedbackParam {
	uint8_t position;	/*E position where the strength of target trigger start changing(0~9). */
	uint8_t strength;	/*E strength that the motor arm pushes back target trigger(0~8 (0: Same as Off mode)). */
	uint8_t padding[46];
} ScePadTriggerEffectFeedbackParam;

/**
 *E
 *  @brief parameter for setting the trigger effect to Weapon mode.
 *         Weapon Mode: Emulate weapon like gun trigger.
 **/
typedef struct ScePadTriggerEffectWeaponParam {
	uint8_t startPosition;	/*E position where the stiffness of trigger start changing(2~7). */
	uint8_t endPosition;	/*E position where the stiffness of trigger finish changing(startPosition+1~8). */
	uint8_t strength;		/*E strength of gun trigger(0~8 (0: Same as Off mode)). */
	uint8_t padding[45];
} ScePadTriggerEffectWeaponParam;

/**
 *E
 *  @brief parameter for setting the trigger effect to Vibration mode.
 *         Vibration Mode: Vibrates motor arm around specified position.
 **/
typedef struct ScePadTriggerEffectVibrationParam {
	uint8_t position;	/*E position where the motor arm start vibrating(0~9). */
	uint8_t amplitude;	/*E vibration amplitude(0~8 (0: Same as Off mode)). */
	uint8_t frequency;	/*E vibration frequency(0~255[Hz] (0: Same as Off mode)). */
	uint8_t padding[45];
} ScePadTriggerEffectVibrationParam;

/**
 *E
 *  @brief parameter for setting the trigger effect to ScePadTriggerEffectMultiplePositionFeedbackParam mode.
 *         Multi Position Feedback Mode: The motor arm pushes back trigger.
 *                                       Trigger obtains specified stiffness at each control point.
 **/
typedef struct ScePadTriggerEffectMultiplePositionFeedbackParam {
	uint8_t strength[SCE_PAD_TRIGGER_EFFECT_CONTROL_POINT_NUM];	/*E strength that the motor arm pushes back target trigger at position(0~8 (0: Same as Off mode)).
																 *  strength[0] means strength of motor arm at position0.
																 *  strength[1] means strength of motor arm at position1.
																 *  ...
																 * */
	uint8_t padding[38];
} ScePadTriggerEffectMultiplePositionFeedbackParam;

/**
 *E
 *  @brief parameter for setting the trigger effect to Feedback3 mode.
 *         Slope Feedback Mode: The motor arm pushes back trigger between two spedified control points.
 *                              Stiffness of the trigger is changing depending on the set place.
 **/
typedef struct ScePadTriggerEffectSlopeFeedbackParam {

	uint8_t startPosition;	/*E position where the strength of target trigger start changing(0~endPosition). */
	uint8_t endPosition; 	/*E position where the strength of target trigger finish changing(startPosition+1~9). */
	uint8_t startStrength;	/*E strength when trigger's position is startPosition(1~8) */
	uint8_t endStrength;	/*E strength when trigger's position is endPosition(1~8) */
	uint8_t padding[44];
} ScePadTriggerEffectSlopeFeedbackParam;

/**
 *E
 *  @brief parameter for setting the trigger effect to Vibration2 mode.
 *         Multi Position Vibration Mode: Vibrates motor arm around specified control point.
 *                                        Trigger vibrates specified amplitude at each control point.
 **/
typedef struct ScePadTriggerEffectMultiplePositionVibrationParam {
	uint8_t frequency;												/*E vibration frequency(0~255 (0: Same as Off mode)) */
	uint8_t amplitude[SCE_PAD_TRIGGER_EFFECT_CONTROL_POINT_NUM];	/*E vibration amplitude at position(0~8 (0: Same as Off mode)).
																	 *  amplitude[0] means amplitude of vibration at position0.
																	 *  amplitude[1] means amplitude of vibration at position1.
																	 *  ...
																	 * */
	uint8_t padding[37];
} ScePadTriggerEffectMultiplePositionVibrationParam;

/**
 *E
 *  @brief parameter for setting the trigger effect mode.
 **/
typedef union ScePadTriggerEffectCommandData {
	ScePadTriggerEffectOffParam							offParam;
	ScePadTriggerEffectFeedbackParam					feedbackParam;
	ScePadTriggerEffectWeaponParam						weaponParam;
	ScePadTriggerEffectVibrationParam					vibrationParam;
	ScePadTriggerEffectMultiplePositionFeedbackParam	multiplePositionFeedbackParam;
	ScePadTriggerEffectSlopeFeedbackParam				slopeFeedbackParam;
	ScePadTriggerEffectMultiplePositionVibrationParam	multiplePositionVibrationParam;
} ScePadTriggerEffectCommandData;

/**
 *E
 *  @brief parameter for setting the trigger effect.
 **/
typedef struct ScePadTriggerEffectCommand {
	ScePadTriggerEffectMode mode;
	uint8_t padding[4];
	ScePadTriggerEffectCommandData commandData;
} ScePadTriggerEffectCommand;

/**
 *E
 *  @brief parameter for the scePadSetTriggerEffect function.
 **/
typedef struct ScePadTriggerEffectParam {

	uint8_t triggerMask;		/*E Set trigger mask to activate trigger effect commands.
								 *  SCE_PAD_TRIGGER_EFFECT_TRIGGER_MASK_L2 : 0x01
								 *  SCE_PAD_TRIGGER_EFFECT_TRIGGER_MASK_R2 : 0x02
								 * */
	uint8_t padding[7];

	ScePadTriggerEffectCommand command[SCE_PAD_TRIGGER_EFFECT_TRIGGER_NUM];	/*E command[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_L2] is for L2 trigger setting
																			 *  and param[SCE_PAD_TRIGGER_EFFECT_PARAM_INDEX_FOR_R2] is for R2 trgger setting.
																			 * */
} ScePadTriggerEffectParam;

#if defined(__cplusplus) && __cplusplus >= 201103L
static_assert(sizeof(ScePadTriggerEffectParam) == 120, "ScePadTriggerEffectParam has incorrect size");
#endif

#endif // _SCE_PAD_TRIGGER_EFFECT_H

#ifndef ORIGINAL_LIBSCEPAD_STRUCTS_H
#define ORIGINAL_LIBSCEPAD_STRUCTS_H

enum s_ScePadDeviceClass {
	SCE_PAD_DEVICE_CLASS_INVALID = -1,
	SCE_PAD_DEVICE_CLASS_STANDARD = 0,
	SCE_PAD_DEVICE_CLASS_GUITAR = 1,
	SCE_PAD_DEVICE_CLASS_DRUM = 2,
	SCE_PAD_DEVICE_CLASS_DJ_TURNTABLE = 3,
	SCE_PAD_DEVICE_CLASS_DANCEMAT = 4,
	SCE_PAD_DEVICE_CLASS_NAVIGATION = 5,
	SCE_PAD_DEVICE_CLASS_STEERING_WHEEL = 6,
	SCE_PAD_DEVICE_CLASS_STICK = 7,
	SCE_PAD_DEVICE_CLASS_FLIGHT_STICK = 8,
	SCE_PAD_DEVICE_CLASS_GUN = 9,
};

typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} s_SceLightBar;

typedef struct {
	uint8_t X;
	uint8_t Y;
} s_SceStickData;

struct s_SceFQuaternion {
	float x, y, z, w;
};

struct s_SceFVector3 {
	float x, y, z;
};

struct s_ScePadTouch {
	uint16_t x;
	uint16_t y;
	uint8_t id;
	uint8_t reserve[3];
};

struct s_ScePadTouchData {
	uint8_t touchNum;
	uint8_t reserve[3];
	uint32_t reserve1;
	s_ScePadTouch touch[2];
};

struct s_ScePadExtensionUnitData {
	uint32_t extensionUnitId;
	uint8_t reserve[1];
	uint8_t dataLength;
	uint8_t data[10];
};

struct s_ScePadVibrationParam {
	uint8_t largeMotor;
	uint8_t smallMotor;
};

struct s_ScePadVolumeGain {
	uint8_t speakerVolume;
	uint8_t headsetVolume;
	uint8_t padding;
	uint8_t micGain;
};

struct s_ScePadTouchPadInformation {
	float pixelDensity;

	struct {
		uint16_t x;
		uint16_t y;
	} resolution;
};

struct s_ScePadStickInformation {
	uint8_t deadZoneLeft;
	uint8_t deadZoneRight;
};


struct s_ScePadInfo {
	s_ScePadTouchPadInformation touchPadInfo;
	s_ScePadStickInformation stickInfo;
	uint8_t connectionType;
	uint8_t connectedCount;
	bool connected;
	s_ScePadDeviceClass deviceClass;
	uint8_t reserve[8];
};

typedef struct {
	uint32_t ExtensionUnitId;
	uint8_t Reserve;
	uint8_t DataLen;
	uint8_t Data[10];
} s_ScePadExtUnitData;

typedef struct {
	uint32_t bitmask_buttons;
	s_SceStickData LeftStick;
	s_SceStickData RightStick;
	uint8_t L2_Analog;
	uint8_t R2_Analog;
	uint16_t padding;
	s_SceFQuaternion orientation;
	s_SceFVector3 acceleration;
	s_SceFVector3 angularVelocity;
	s_ScePadTouchData touchData;
	bool connected;
	uint64_t timestamp;
	s_ScePadExtUnitData extUnitData;
	uint8_t connectionCount;
	uint8_t reserved[2];
	uint8_t deviceUniqueDataLen;
	uint8_t deviceUniqueData[12];

} s_ScePadData;

#pragma pack(push, 1)
struct s_ScePadContainerIdInfo {
	uint32_t size;
	char id[0x2000]; // UTF-8 string
};
#pragma pack(pop)

enum s_SceControllerType {
	UNKNOWN = 0,
	DUALSHOCK_4 = 1,
	DUALSENSE = 2
};

#endif // ORIGINAL_LIBSCEPAD_STRUCTS_H

#define SCE_OK 0

// Buttons
#define SCE_BM_CROSS    0x00004000
#define SCE_BM_CIRCLE   0x00002000
#define SCE_BM_TRIANGLE 0x00001000
#define SCE_BM_SQUARE   0x00008000
#define SCE_BM_L1       0x00000400
#define SCE_BM_L2       0x00000100
#define SCE_BM_R1       0x00000800
#define SCE_BM_R2       0x00000200
#define SCE_BM_L3       0x00000002
#define SCE_BM_R3       0x00000004
#define SCE_BM_N_DPAD   0x00000010
#define SCE_BM_S_DPAD   0x00000040
#define SCE_BM_E_DPAD   0x00000020
#define SCE_BM_W_DPAD   0x00000080
#define SCE_BM_OPTIONS  0x00000008
#define SCE_BM_TOUCH    0x00100000
#define SCE_BM_SHARE    0x00000001
#define SCE_BM_PSBTN    0x00010000
#define SCE_BM_MICBUTTON 0x00020000

// Errors
#define SCE_PAD_ERROR_INVALID_ARG              0x80920001
#define SCE_PAD_ERROR_INVALID_PORT             0x80920002
#define SCE_PAD_ERROR_INVALID_HANDLE           0x80920003
#define SCE_PAD_ERROR_ALREADY_OPENED           0x80920004
#define SCE_PAD_ERROR_NOT_INITIALIZED          0x80920005
#define SCE_PAD_ERROR_INVALID_LIGHTBAR_SETTING 0x80920006
#define SCE_PAD_ERROR_DEVICE_NOT_CONNECTED     0x80920007
#define SCE_PAD_ERROR_NO_HANDLE                0x80920008
#define SCE_PAD_ERROR_FATAL                    0x809200FF
#define SCE_PAD_ERROR_NOT_PERMITTED            0x80920101
#define SCE_PAD_ERROR_INVALID_BUFFER_LENGTH    0x80920102
#define SCE_PAD_ERROR_INVALID_REPORT_LENGTH    0x80920103
#define SCE_PAD_ERROR_INVALID_REPORT_ID        0x80920104
#define SCE_PAD_ERROR_SEND_AGAIN               0x80920105

// Connection types
#define SCE_PAD_CONNECTION_TYPE_LOCAL              0
#define SCE_PAD_CONNECTION_TYPE_REMOTE_VITA        1
#define SCE_PAD_CONNECTION_TYPE_REMOTE_DUALSHOCK4  2

// scePadIsControllerUpdateRequired output
#define SCE_PAD_UPDATE_NOT_REQUIRED 0
#define SCE_PAD_UPDATE_REQUIRED 1

// Audio paths
#define SCE_PAD_AUDIO_PATH_STEREO_HEADSET 0x00
#define SCE_PAD_AUDIO_PATH_MONO_LEFT_HEADSET 0x01
#define SCE_PAD_AUDIO_PATH_MONO_LEFT_HEADSET_AND_SPEAKER 0x02
#define SCE_PAD_AUDIO_PATH_ONLY_SPEAKER 0x03

// Vibration modes
#define SCE_PAD_HAPTICS_MODE 0x01
#define SCE_PAD_RUMBLE_MODE 0x02

// Bus types
#define SCE_PAD_BUSTYPE_USB 1
#define SCE_PAD_BUSTYPE_BT 2

struct s_ScePadInitParam {
	uint8_t  customAllocAndFree[16]; // Can be left unused
	uint32_t allowBT;         // Set to 1 to allow Bluetooth connections, 0 to disable
	uint64_t  padding1[2];      
	uint32_t  padding2;       
};

#if defined(_WIN32) || defined(_WIN64)
	#ifdef DUALIB_EXPORTS
		#define DUALIB_API __declspec(dllexport)
		#else
		#define DUALIB_API __declspec(dllimport)
	#endif
#else
	#ifdef DUALIB_EXPORTS
		#define DUALIB_API __attribute__((visibility("default")))
		#else
		#define DUALIB_API
	#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Use scePadInit3() if you want to allow for bluetooth connections
 int scePadInit();
 int scePadInit3(s_ScePadInitParam* param);
 int scePadTerminate();
 int scePadOpen(int userID, int unk1, int unk2);
 int scePadSetParticularMode(bool mode);
 int scePadGetParticularMode();
 int scePadReadState(int handle, s_ScePadData* data);
 int scePadGetContainerIdInformation(int handle, s_ScePadContainerIdInfo* containerIdInfo);
 int scePadSetLightBar(int handle, s_SceLightBar* lightbar);
 int scePadGetHandle(int userID, int unk1, int unk2);
 int scePadResetLightBar(int handle);
 int scePadSetTriggerEffect(int handle, ScePadTriggerEffectParam* triggerEffect);
 int scePadGetControllerBusType(int handle, int* busType);
 int scePadGetControllerInformation(int handle, s_ScePadInfo* info);
 int scePadGetControllerType(int handle, s_SceControllerType* controllerType);
 int scePadGetJackState(int handle, int* state);
 int scePadGetTriggerEffectState(int handle, int state[2]);
 int scePadIsControllerUpdateRequired(int handle);
/// Don't use this. Use scePadReadState instead
 int scePadRead(int handle, s_ScePadData* data, int count);
 int scePadResetOrientation(int handle);
 int scePadSetAngularVelocityDeadbandState(int handle, bool state);
 int scePadSetAudioOutPath(int handle, int path);
 int scePadSetMotionSensorState(int handle, bool state);
 int scePadSetTiltCorrectionState(int handle, bool state);
 int scePadSetVibration(int handle, s_ScePadVibrationParam* vibration);
 int scePadSetVibrationMode(int handle, int mode);
 int scePadSetVolumeGain(int handle, s_ScePadVolumeGain* gainSettings);
 int scePadIsSupportedAudioFunction(int handle);
 int scePadClose(int handle);

 int scePadSetPlayerLedBrightness(int handle, int brightness);
 int scePadSetPlayerLed(int handle, bool state);
 std::string scePadGetMacAddress(int handle);
 std::string scePadGetPath(int handle);
 int scePadSetTriggerEffectCustom(int handle, uint8_t left[11], uint8_t right[11], uint8_t triggerBitmask);
 void* scePadGetHidApiHandle(int handle);
#ifdef __cplusplus
}
#endif

#endif // DUALIB_H