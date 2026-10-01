Lumia Rift Arcade + PSP Lab

A Windows 10 Mobile / UWP ARM32 phone-game project for the Lumia 950 family.

GAME
Lumia Rift is a self-contained neon arcade shooter with a moving starfield, enemy waves, combo scoring, boost, particles, touch controls, and desktop keyboard controls.

PSP VAULT
The app asks for in-app consent, opens the normal Windows FileOpenPicker, filters to .iso, never scans storage in the background, can remember the chosen file with FutureAccessList, and tries Launcher.LaunchFileAsync so an installed PSP/ISO handler can receive it.

The repository also pins basharast/PPSSPP-UWP-ARM as a Git submodule. That is the actual ARM32 UWP PSP emulator codebase; the Lumia C# app does not pretend to implement a PSP CPU/GPU emulator itself.

REALISTIC LIMITATION
The game shell and PSP front end are complete, but this branch does not claim that PPSSPP and the game have been fused into one executable. Fully embedding the native PPSSPP core requires substantial C++ integration plus testing on a real Windows 10 Mobile ARM32 device. The submodule and legacy build workflow provide the correct foundation.

PERMISSIONS
No broad storage capability is declared. The app explains the request first, then Windows presents the picker. Only the file the user chooses is accessed.

BUILD
Visual Studio 2022 with UWP tools and Windows 10 SDK 19041.
Platform: ARM
Configuration: Release
Minimum: Windows 10 build 14393

Run: git submodule update --init --recursive
Open: LumiaRiftGame.sln
Or run: .\\Build-All.ps1

GitHub Actions builds an unsigned development package. Device sideloading still requires the appropriate developer-unlock/trust setup.

LEGAL
The original Lumia Rift code and logo in this folder are MIT licensed. PPSSPP remains under its own GPL license in the submodule. Only load PSP software you are legally entitled to use.
