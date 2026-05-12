# ESP32 Joypad Bridge

This extra subproject adds wireless Bluetooth controller support to the FWKES emulator using an ESP32 microcontroller as a bridge between a Bluetooth gamepad and the emulator's NES controller interface.

For example, you can connect:
- Xbox controllers
- PlayStation controllers
- Generic Bluetooth gamepads
- Other controllers supported by Bluepad32

The ESP32 communicates with the emulator through the MINI-DIN 5 connector and behaves like a standard NES controller.

This module is fully external and does **not** require any modifications to the main emulator hardware or firmware.

You can still use:
- original NES controllers
- or this wireless ESP32 bridge


---

# Requirements

## Hardware
You will need:

1. ESP32 development board with Bluetooth support
2. Bluetooth controller/gamepad
3. MINI-DIN connector (optional or just via wires connect directly)
4. Jumper wires

---

# Software Requirements

Install the following in Arduino IDE:

1. ESP32 board support package
2. Bluepad32 library


---

# Setup

1. Open the `.ino` project from this repository in Arduino IDE
2. Install all required libraries
3. Flash the firmware to your ESP32
4. Connect the following signals between the emulator and ESP32:

| Emulator Signal | ESP32 Pin |
|---|---|
| STROBE / LATCH | User defined |
| CLOCK          | User defined |
| DATA OUT       | User defined |
| GND            | GND |

5. Power on the ESP32
6. Pair your Bluetooth controller
7. Enjoy wireless gameplay


---

# Notes

- The bridge emulates a standard NES controller shift register interface
- No emulator-side software changes are required
- Button mapping can be modified directly in the source code
- Timing-sensitive communication is handled using hardware interrupts


---

# Credits

Built using:
- ESP32
- Bluepad32
- Custom NES serial protocol emulation