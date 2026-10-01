# Commodore Warp UWP

C64-first Windows 10 Mobile/UWP ARM32 Commodore workstation.

## What it does

C64 is the default machine, with a C128 toggle. The emulator host uses the VICE libretro core.

The CPU control now ranges from 1 MHz through 64 MHz effective emulation speed. Internally this is frontend fast-forward: higher values run more VICE frames between screen updates. It does not claim that the emulated VIC-II/CIA/CPU timing constants have been physically changed to a real 64 MHz C64.

The hold button still provides a fixed 16x quick Warp mode.

## Program Lab

Write C64 BASIC directly on the phone.

- New program
- Load .BAS or .TXT
- Save source locally
- Save source to the SD workspace
- Export a tokenized C64 .PRG using the normal $0801 BASIC load address
- Export the generated .PRG directly to SD

The exporter assigns 10,20,30... line numbers when a source line has none, tokenizes a common C64 BASIC keyword/operator set, preserves strings and REM text, sorts lines, and uses the last copy of a duplicate line number.

## SD card

Press ENABLE SD first. The app displays an in-app confirmation. After approval it uses UWP removable-device storage access and creates a CommodoreWarp folder on the first removable device it can access. The folder is stored in the FutureAccessList for later access.

Microsoft documents that KnownFolders.RemovableDevices represents removable devices, and direct enumeration/creation/change requires the removableStorage capability plus file-type associations for the file types an app accesses. The manifest declares these.

If no card is present, local program storage continues to work.

## Files

The main software picker accepts .prg, .d64, .g64, .d81, .t64, .tap, .crt and .zip. Selected software is copied into LocalFolder before the VICE core loads it.

## Build

Use Visual Studio 2022 with Desktop C++, UWP tools, and Windows 10 SDK 19041.

Build Release | ARM.

The VICE source is referenced by the git submodule in .gitmodules and pinned in ViceSourcePin.txt. Build ARM32 VICE cores named vice_x64_libretro.dll and vice_x128_libretro.dll and place them in the app Cores folder.

Only use software, disk images, and ROM material you are legally entitled to use.


ARM32 package CI uses single-node MSBuild to keep Visual Studio UWP/XAML builds deterministic.
