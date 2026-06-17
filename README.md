![FWKES logo](./logo.png)

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
  
<img width="1939" height="1089" alt="fwkes" src="https://github.com/user-attachments/assets/1945d1e4-d2a4-43a1-9c76-a94c4f0aaa1a" />
<img width="1939" height="1089" alt="pcb_low" src="https://github.com/user-attachments/assets/4506caa3-f269-4394-a9f8-f8b2226fdd8f" />
<img width="6000" height="3368" alt="fwkes_back" src="https://github.com/user-attachments/assets/4808bebb-d9c0-4904-bde5-d94cf89d2ea7" />


# Building

Although FWKES is aimed at RP2350-based boards, there are development builds for desktop. Desktop
uses SDL3 to handle video, audio and inputs, and it's intended for superficial testing of
non-RP2350-specific aspects.

There is also an ongoing development of debugger, which allows complex testing not only of FWKES
itself, but also emulated programs and games. It's code can be found in the `debugger` branch.

## Dependencies

For RP2350 based, you'll need [ARM GCC compiler toolchain with C11
support](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) and [extracted Pico
SDK](https://github.com/raspberrypi/pico-sdk).

For desktop and debugger versions, only [SDL3](https://www.libsdl.org/) is required to be installed
and usable by CMake. Both GCC and Clang are usable.

For general building, you'll also need [CMake](https://cmake.org/) and [Git](https://git-scm.com/).

Many of listed tools are available in distribution's repositories on Linux.

## Cloning

Since most dependencies are handled using Git submodules, you need to use `git clone --recurse-submodules
https://github.com/Inu-Labs/fwkes.git`. Alternatively, if you already cloned and submodules are not
initialized, you need to run `git submodule update --init --recursive --progress`.

## Raspberry Pi Pico 2

`-DBUILD_RP2350=ON` must be specified when compiling for official Pico 2:

```sh
mkdir build
cd build
cmake -DPICO_SDK_PATH=path_to_extracted_pico-sdk -DCMAKE_BUILD_TYPE=Release -GNinja -DBUILD_RP2350=ON ..
```

## Pimoroni Pico Plus 2

Both `-DBUILD_RP2350=ON` and `-DBUILD_PIMORONI2=ON` must be specified when compiling for Pimoroni 2:

```sh
mkdir build
cd build
cmake -DPICO_SDK_PATH=path_to_extracted_pico-sdk -DCMAKE_BUILD_TYPE=Release -GNinja -DBUILD_RP2350=ON -DBUILD_PIMORONI2=ON ..
```

## Desktop

Pass `-DBUILD_DESKTOP=ON`:

```sh
mkdir build
cd build
cmake -GNinja -DBUILD_DESKTOP=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ..
```

## Debugger

Pass both `-DBUILD_DESKTOP=ON` and `-DBUILD_DEBUGGER=ON`:

```sh
mkdir build
cd build
cmake -GNinja -DBUILD_DESKTOP=ON -DBUILD_DEBUGGER=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ..
```

## Tests (development only)

To build tests, pass `-DBUILD_TESTS=ON` and `-DBUILD_VISUAL_TESTS=ON`:

```sh
mkdir build
cd build
cmake -GNinja -DBUILD_DESKTOP=ON -DBUILD_TESTS=ON -DBUILD_VISUAL_TESTS=ON ..
```

`-DBUILD_VISUAL_TESTS=ON` is required for tests that depend on SDL3.

## Flashing firmware

To flash compiled firmware to RP2350 board, you can either:

- Copy `build/src/rp2350/fwkes-rp2350.uf2` to the board. Note that the board must be booted in the BOOTSEL mode to perform this (otherwise it will not even appear in your file manager).
- Use [picotool](https://github.com/raspberrypi/picotool): `picotool load build/src/rp2350/fwkes-rp2350.uf2 -f && picotool reboot`

A CMake rule for this is planned in the future.

## Electronics, case

*TODO*

# License & Acknowledgement

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
