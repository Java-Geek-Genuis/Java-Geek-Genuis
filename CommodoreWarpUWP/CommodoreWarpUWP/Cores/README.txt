Place the ARM32 VICE libretro DLLs in this folder before packaging:
vice_x64_libretro.dll for C64
vice_x128_libretro.dll for C128

Build the VICE source submodule with the legacy UWP ARM target when your Visual Studio installation provides the required toolchain. The app's Build project includes any DLLs found in Cores/.
