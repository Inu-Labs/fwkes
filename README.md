![FWKES logo](./logo.svg)

FWKES[^1] is an open-source NES emulator targeted at 32-bit microcontroller RP2350. It aims to be
precise and cycle-accurate, while still maintaining stable 60 FPS and minimal latency.

*TODO: add demonstration (video, screenshots)*

Features:

- SD card for storing games and files.
- HDMI for video output.
- 3.5mm audio jack for audio output.
- Supports raster effects (mid-scanline writes).
- Emulates NTSC region, i.e. original NES and Famicom (although support for PAL is planned).
- Implements all instruction set of Ricoh 2A03 (MOS 6502), including unofficial instructions and
precise timing.
- Implements all features of PPU (sprite overflow, emphasis bits, ...) and APU.
- Supports **NROM**, **MMC1**, **UNROM** and **MMC3** mappers.
- Supports RP2350-based boards **Raspberry Pi Pico 2**, **Pimoroni Pico Plus 2** and any compatible
board.
- FWKES comes with BIOS that delivers user-friendly interface for loading programs and
  configuration. BIOS can be freely reprogrammed, i.e. can replace it with their own program.
- To allow emulated programs make use of peripherals like SD card, or configure emulator's behavior,
  there is a bridge (hypercall interface) called FWX.
  
<img width="559" alt="IMG_20260512_191624" src="https://github.com/user-attachments/assets/1097341c-2204-4e90-8227-d03ecbf42125" />
<img width="508" alt="motherboard" src="https://github.com/user-attachments/assets/36ba81ca-0e24-46b2-9c1b-fc83861de646" />


## Building

*TODO*

## License & Acknowledgement

Code of FWKES is licensed under [GNU GPL v2.0](LICENSE.txt), while hardware (schematics, PCB designs
and 3D models) is licensed under [CERN Open Hardware Licence Version 2 (Strongly
Reciprocal)](./CERN-OHL-S-2.0-LICENSE.txt). This project is a collaborative work of
[inunix3](https://github.com/inunix3) and [kubaa-ma](https://github.com/kubaa-ma).

Software side of FWKES makes use of the following libraries:

- [pico-sdk](https://github.com/raspberrypi/pico-sdk) - not too low-level interaction with RP2350 peripherals.
- [PicoDVI](https://github.com/Wren6991/PicoDVI) - generation of bitbanged DVI signal.
- [LwMEM](https://github.com/MaJerle/lwmem) - allocator interface for embedded systems (used for PSRAM).
- [FatFS](https://elm-chan.org/fsw/ff/) - FAT filesystem implementation for SD card.
- [Unity](https://github.com/ThrowTheSwitch/Unity) - unit-testing framework (used for CPU tests).
- [ImGui](https://github.com/ocornut/imgui.git) and [imgui_club](https://github.com/ocornut/imgui_club.git) - used for debugger UI.
- [ImGuiFileDialog](https://github.com/aiekick/ImGuiFileDialog) - used for open/save file dialog in the debugger.

[^1]: FWKES is pronounced as fukes (/fʌkɛs/). In old Bible printings, you can find that sometimes
    letter U is written as V. So we took it further and doubled it! Although it's not read as double
    U...
