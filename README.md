[Legacy C# version](https://github.com/WujekFoliarz/DualSenseY/releases/tag/6.2) 

### Download → https://github.com/WujekFoliarz/DualSenseY-v2/releases
### Discord → https://discord.gg/AFYvxf282U
### If you want to buy me a coffee → https://ko-fi.com/wujekfoliarz
### Translations → https://crowdin.com/project/dualsensey (If your language is not there, create an issue on github or contact me on discord)

# Is this related to DSX?
 - No, it's a free alternative for those who need it and a hobby project.

# How do I activate UDP?
 - All you need to do is run a game with dualsense mod installed, it will turn to active as soon as it receives data (If the mod asks for a port, use 6969)

# Problems:
 - I get a pop-up that says ViGEmBus is not installed! -> https://github.com/nefarius/ViGEmBus/releases/download/v1.22.0/ViGEmBus_1.22.0_x64_x86_arm64.exe
 - Controller emulation is wonky? Install .NET 8.0 -> https://aka.ms/dotnet-core-applaunch?missing_runtime=true&arch=x86&rid=win-x86&os=win10&apphost_version=8.0.6
 - Double input in-game? Install HidHide driver and run app as admin -> https://github.com/nefarius/HidHide/releases/download/v1.5.230.0/HidHide_1.5.230_x64.exe
 - The app renders a black screen on my laptop! -> Run the application with your dedicated GPU instead of iGPU

# DualSenseY

**DualSenseY** is a powerful tool designed for controlling and customizing Sony DualSense (PS5) controllers on Windows. It features advanced capabilities for LED control, adaptive trigger configuration, haptics, microphone controls, and even screen capture functionality via the controller's microphone button.

# Features

## Multiplayer
 - Easily connect any amount of DualSense controllers, not found in other free apps
 - Run Xbox 360/DualShock 4 controller emulation on any of them with ease and consistency
 
## Controller Functionality
- **R2 to Mouse Click**: Map the R2 trigger to mouse click.
- **L2 and R2 Deadzones**: Configure deadzones for triggers.
- **Minimum Stick Value**: Set the minimum value for analog sticks.
- **Gyro to Mouse**: Use the gyroscope to control the mouse.
- **Gyro to Right Analog Stick**: Map gyroscope movements to the right analog stick.
- **Touchpad to Mouse**: Use the touchpad as a laptop-style touchpad.
- **Touchpad to Haptics**: Vibrate the controller based on finger position.
- **Use Touchpad as Right Stick**: Map the touchpad to the right analog stick.

## Adaptive Triggers and Haptics
- **Haptics to Adaptive Triggers**: Translate haptic feedback into trigger effects.
- **Rumble to Adaptive Triggers**: Convert rumble vibrations to trigger feedback.
- **Rigid Trigger Mode**: Set rigid trigger effects.
- **Maximum Intensity and Frequency**: Adjust trigger vibration sensitivity and frequency.

## LED and Lightbar Controls
- **Battery Status Lightbar**: Display battery level with LED colors.
- **Disco Mode**: Animated color transitions for the lightbar.
- **Audio to LED**: Sync lightbar with audio levels.
- **Quiet, Medium, and Loud Colors**: Customize lightbar colors based on audio volume.

## Emulation and Shortcuts
- **Start/Stop X360 and DS4 Emulation**: Enable or disable emulation modes.
- **Trigger as Buttons**: Map triggers as buttons with hard resistance.
- **Mic Button Shortcuts**:
  - Toggle touchpad modes.
  - Start/Stop emulation modes.
  - Swap triggers for adaptive feedback.

## Audio and Speaker Integration
- **Controller Speaker Volume**: Adjust the hardware-level volume of the controller speaker.
- **Audio to Haptics**: Create haptic feedback from system audio.
- **Run Audio to Haptics on Startup**: Automatically enable audio-based haptics.

## Configuration and Settings
- **Save/Load Configuration**: Manage configuration files.
- **Set/Remove Default Configurations**: Auto-load settings for specific ports.
- **Style Customization**:
  - Save/Load styles.
  - Set/Remove default styles.
  - Reset styles to default.

## Motion Controls
- **Gyroscope and Accelerometer**: Enable and configure motion controls.
- **Motion Sensitivity**: Adjust sensitivity for motion inputs.
- **Gyro Deadzone**: Configure deadzones for gyroscopic inputs.

## Miscellaneous
- **Screenshot Functionality**: Save screenshots using the microphone button.
- **Connection Type and Battery Info**: View connection type and battery level.

## Libraries used

  - [Dear ImGui](https://github.com/ocornut/imgui) for the graphical interface.
  - [GLFW](https://www.glfw.org/) and **OpenGL** for rendering.
  - [miniaudio](https://github.com/mackron/miniaudio) for audio processing.
  - [ViGEmClient](https://github.com/nefarius/ViGEmClient) for controller emulation.
  - [libcpr](https://github.com/libcpr/cpr)
  - [traypp](https://github.com/Soundux/traypp)
  - [Crashlogs](https://github.com/TylerGlaiel/Crashlogs)
  - [hidapi](https://github.com/libusb/hidapi)
  - [nlohmann/json](https://github.com/nlohmann/json)

## Usage

1. **Start DualSenseY** and connect your DualSense controller.
2. **Select Controller**: Choose the controller you want to configure from the dropdown list.
3. **Configure LED, Haptics, and Adaptive Triggers**:
4. **Microphone Controls**: Use the microphone button for screenshots or muting the built-in microphone.
5. **Controller Emulation**: Switch between Xbox 360 and DualShock 4 emulation modes as needed.
6. **Save/Load Configurations**:
   - Save your current configuration as a file or load a previous configuration.
   - Set default configurations for specific ports.
