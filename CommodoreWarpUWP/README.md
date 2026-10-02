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

Packaging branch trigger.

CI package definition updated.


XAML dependent metadata fixed for ARM32 packaging.


ARM32 packaging now builds and embeds the pinned VICE x64 and x128 libretro cores and verifies both are inside the final AppX.


Build tooling: MSYS2 make is installed in CI for the VICE ARM32 compatibility build.


VICE ARM32 Makefile patch corrected: custom platform branch now participates in the existing VICE platform chain without prematurely closing it.


VICE ARM32 builder now defines the DLL target and applies MSVC flag filtering after VICE's platform/core flags are assembled.


CI toolchain fix: MSYS2 inherits the Visual Studio ARM32 environment so VICE can invoke cl.exe.


Verified CI revision: MSYS2 inherits the Visual Studio ARM32 environment before VICE compilation.


Final CI toolchain setting: MSYS2 path-type is explicitly `inherit` so Visual Studio ARM32 cl.exe is visible to GNU make.


Absolute ARM32 cl.exe path is embedded into the generated VICE makefile so MSYS2 PATH mode cannot hide the compiler.


Final CI pass uses a single branch-wide concurrency slot and an absolute Visual Studio ARM32 compiler path.


ARM32 VICE compatibility now forces Hostx64\arm\cl.exe and pre-includes msvc_compat.h for GCC builtin compatibility.


Final Windows SDK compatibility: VICE MSVC ARM32 build explicitly defines _ARM_ and _M_ARM for winnt.h architecture detection.


VICE ARM32 MSVC now defines NOMINMAX to avoid Windows min/max macro collisions in reSID.


Final emulator path: official Libretro ARM32 VICE cores are preferred; framebuffer upload is handled through the UWP PixelBuffer ABI.


Build speed: VICE ARM32 source compilation uses two make workers.


Test-package note: the Actions workflow uses preserved ARM32/MSVC VICE DLLs from the public RetroArch-ARM archive and verifies their PE architecture before AppX packaging.


Packaging fix: VICE C64/C128 DLLs are explicit DeploymentContent entries in the UWP project so MSBuild includes them in the AppX.
