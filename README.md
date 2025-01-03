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

## Features

- **Multiplayer**
   - Easily connect any amount of DualSense controllers, not found in other free apps
   - Run Xbox 360/DualShock 4 controller emulation on any of them with ease and consistency

- **LED Customization**:
  - Set custom RGB values for the controller’s LED light bar.
  - Enable **Disco Mode** for animated color transitions.
  - Sync the LED color with audio levels using **Audio to LED**.
  - Display **Battery Status** with color-coded LED indicators.

- **Adaptive Trigger Control**:
  - Customize the left and right triggers with various feedback modes, including **Rigid**, **Pulse**, and **Calibration**.
  - Enable **Rumble to Adaptive Triggers** for synchronized trigger feedback.
  
- **Haptic Feedback**:
  - Control left and right motor rumble levels for traditional haptic feedback.
  - Enable **Audio to Haptics** for haptic feedback generated from system audio (USB-only).
  
- **Touchpad Visualization**:
  - Display real-time touch inputs and coordinates on the controller’s touchpad.

- **Microphone Button Functionality**:
  - **Screenshot Capture**: Take screenshots with a single press of the microphone button.
  - **Mic Function Emulation**: Toggle microphone LED and volume with button presses.

- **Controller Emulation**:
  - Emulate as an **Xbox 360** or **DualShock 4** controller using ViGEmBus.
  - **Hide/Show Real Controller** with Hidhide integration, allowing you to manage visibility in games and other applications.

- **UDP Connectivity**:
  - Use UDP to control settings remotely and synchronize with other software or devices.

## Requirements (For programmers)

- **Libraries**
  - [Dear ImGui](https://github.com/ocornut/imgui) for the graphical interface.
  - [GLFW](https://www.glfw.org/) and **OpenGL** for rendering.
  - [miniaudio](https://github.com/mackron/miniaudio) for audio processing.
  - [ViGEm](https://github.com/nefarius/ViGEmClient) for controller emulation.
  - [libcpr](https://github.com/libcpr/cpr)
  - [traypp](https://github.com/Soundux/traypp)
  - [Crashlogs](https://github.com/TylerGlaiel/Crashlogs)
  - **Windows-only**: Uses the Windows API for audio endpoint management and screen capture.

## Usage

1. **Start DualSenseY** and connect your DualSense controller.
2. **Select Controller**: Choose the controller you want to configure from the dropdown list.
3. **Configure LED, Haptics, and Adaptive Triggers**:
4. **Microphone Controls**: Use the microphone button for screenshots or muting the built-in microphone.
5. **Controller Emulation**: Switch between Xbox 360 and DualShock 4 emulation modes as needed.
6. **Save/Load Configurations**:
   - Save your current configuration as a file or load a previous configuration.
   - Set default configurations for specific ports.
