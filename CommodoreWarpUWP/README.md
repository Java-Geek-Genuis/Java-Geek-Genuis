# Commodore Warp UWP

Touch-first Windows 10 Mobile / UWP ARM32 Commodore emulator frontend.

Machines:
- Commodore 64 through the VICE x64 libretro core.
- Commodore 128 through the VICE x128 libretro core.

The speed control is 1x through 16x. Warp 16x is a hold-to-accelerate button. This is frontend fast-forward: multiple emulated frames are executed between display updates. Above 1x the frontend ignores core audio output so fast-forward does not create a huge audio backlog.

Loading uses the Windows file picker and accepts PRG, D64, G64, D81, T64, TAP, CRT and ZIP. The selected file is copied into app LocalState before the core opens it. The app does not scan storage.

The actual emulator code is VICE through the libretro interface. The VICE project supports C64 and C128; its C128 core includes the MMU, VDC 80-column video, fast IEC bus emulation, 2 MHz mode and Z80 support.

The frontend is designed to use an ARM32 VICE libretro DLL placed in Cores/. The DLL is not included as a binary in source control. The ThirdParty/vice-libretro submodule pins the source.

Build:
- Visual Studio 2022
- Desktop C++ and UWP tools
- Windows 10 SDK 19041
- Release | ARM
- minimum Windows 10 build 15063

Open CommodoreWarpUWP.sln.

Only use software and ROM material you are legally entitled to use.
