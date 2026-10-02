param(
    [ValidateSet("x64","x128")]
    [string]$EmuType = "x64"
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$third = Join-Path $root "ThirdParty\vice-libretro"
$pin = "9d7983826ea792f6cce7fdfe6c09488129c6f886"

if (!(Test-Path (Join-Path $third "Makefile"))) {
    New-Item -ItemType Directory -Force (Split-Path $third) | Out-Null
    git clone https://github.com/libretro/vice-libretro.git $third
}

Set-Location $third

$vcvars = Join-Path "$env:ProgramFiles" "Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
if (!(Test-Path $vcvars)) {
    $vcvars = Join-Path "$env:ProgramFiles(x86)" "Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
}
if (!(Test-Path $vcvars)) { throw "Visual Studio vcvarsall.bat was not found." }

git fetch --depth 1 origin $pin
git checkout --detach $pin

$makefile = Get-Content ".\Makefile" -Raw

# Add a real Windows UWP ARM32/MSVC branch to the existing VICE platform
# selection chain.  It deliberately does NOT close the outer if/else chain.
if ($makefile -notmatch "CommodoreWarpUWP ARM32 MSVC") {
    $uwpBlock = @'
# CommodoreWarpUWP ARM32 MSVC
else ifneq (,$(findstring windows_msvc2017_uwp_arm,$(platform)))
    NO_GCC := 1
    WINDOWS_VERSION = 1
    WinPartition = uwp
    TargetArchMoniker = arm
    TARGET := $(TARGET_NAME)_libretro.dll
    MSVC2017CompileFlags = -DWINAPI_FAMILY=WINAPI_FAMILY_APP -D_WINDLL -D_UNICODE -DUNICODE -D__WRL_NO_DEFAULT_LIB__ -D_CRT_SECURE_NO_WARNINGS -DNOMINMAX -EHsc -FS -FImsvc_compat.h
    CFLAGS += $(MSVC2017CompileFlags)
    CXXFLAGS += $(MSVC2017CompileFlags)
    CFLAGS += -D__WIN32__ -D_ARM_ -D_M_ARM=7
    CXXFLAGS += -D__WIN32__ -D_ARM_ -D_M_ARM=7
    COMMONFLAGS += -DNEED_STRCASESTR
    LDFLAGS += -APPCONTAINER -NXCOMPAT -DYNAMICBASE -MANIFEST:NO -OPT:REF -SUBSYSTEM:CONSOLE -MANIFESTUAC:NO -OPT:ICF -ERRORREPORT:PROMPT -NOLOGO -TLBID:1 -DEBUG:FULL -WINMD:NO
    LDFLAGS += WindowsApp.lib
    CXX = cl.exe
    CC = cl.exe
    LD = cl.exe
    fpic :=
'@
    $makefile = $makefile.Replace("# Wincross64", $uwpBlock + [Environment]::NewLine + "# Wincross64")
}

# Apply MSVC cleanup only after the complete flag set has been assembled.
$compat = @'
# CommodoreWarpUWP MSVC output/flag compatibility
ifneq (,$(findstring windows_msvc2017_uwp_arm,$(platform)))
    COMMONFLAGS := $(filter-out -O3 -Wno-format -Wno-format-security -MMD,$(COMMONFLAGS))
    CFLAGS := $(filter-out -Wno-old-style-definition -fPIC,$(CFLAGS))
    CXXFLAGS := $(filter-out -std=c++98 -fPIC,$(CXXFLAGS))
    LDFLAGS := $(filter-out -s -lm -fPIC,$(LDFLAGS))
    OBJOUT = -Fo
    LINKOUT = -Fe
    LD_EXTRA = -LD
else
    OBJOUT = -o
    LINKOUT = -o
    LD_EXTRA =
endif
'@
if ($makefile -notmatch "CommodoreWarpUWP MSVC output/flag compatibility") {
    $makefile = $makefile.Replace("default: info all", $compat + [Environment]::NewLine + "default: info all")
}

# Keep all make recipes unconditional; only output switches vary by platform.
$makefile = $makefile.Replace(
    '$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)',
    '$(CXX) $(LD_EXTRA) $(LINKOUT)$@ $(OBJECTS) $(LDFLAGS)'
)
$makefile = $makefile.Replace(
    '$(CC) $(CFLAGS) -c -o $@ $<',
    '$(CC) $(CFLAGS) -c $(OBJOUT)$@ $<'
)
$makefile = $makefile.Replace(
    '$(CXX) $(CXXFLAGS) -c -o $@ $<',
    '$(CXX) $(CXXFLAGS) -c $(OBJOUT)$@ $<'
)

# Resolve the ARM32 compiler while inside the Visual Studio developer environment
$clCandidates = Get-ChildItem -Path (Join-Path "$env:ProgramFiles" "Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC") -Recurse -Filter "cl.exe" -ErrorAction SilentlyContinue | Where-Object { $_.FullName -match "\\bin\\Hostx64\\arm\\cl\.exe$" } | Sort-Object FullName
if (!$clCandidates -or $clCandidates.Count -eq 0) { throw "Could not find the Visual Studio ARM32 cl.exe." }
$clPath = $clCandidates[-1].FullName.Trim()
$clPathMake = $clPath.Replace("\","/")
$makefile = $makefile.Replace("CC = cl.exe", 'CC = "' + $clPathMake + '"')
$makefile = $makefile.Replace("CXX = cl.exe", 'CXX = "' + $clPathMake + '"')
$makefile = $makefile.Replace("LD = cl.exe", 'LD = "' + $clPathMake + '"')
@'
#pragma once
#ifdef _MSC_VER
#ifndef __builtin_expect
#define __builtin_expect(x,y) (x)
#endif
#ifndef __builtin_expect_with_probability
#define __builtin_expect_with_probability(x,y,p) (x)
#endif
#endif
'@ | Set-Content ".\msvc_compat.h" -Encoding UTF8

Set-Content ".\Makefile.uwp.arm32" $makefile -Encoding UTF8

# Print the key generated section for build diagnostics.
$lineNumber = 0
Get-Content ".\Makefile.uwp.arm32" | ForEach-Object {
    $lineNumber++
    if ($lineNumber -ge 55 -and $lineNumber -le 90) {
        "{0,4}: {1}" -f $lineNumber, $_
    }
}

$make = (Get-Command make.exe -ErrorAction SilentlyContinue).Source
if (!$make) { $make = "C:\msys64\usr\bin\make.exe" }
if (!(Test-Path $make)) { throw "MSYS2 make.exe was not found." }

$cmd = 'call "' + $vcvars + '" x64_arm && set "PATH=C:\msys64\usr\bin;%PATH%" && "' + $make + '" -f Makefile.uwp.arm32 clean && "' + $make + '" -f Makefile.uwp.arm32 EMUTYPE=' + $EmuType + ' platform=windows_msvc2017_uwp_arm'
cmd.exe /d /s /c $cmd
if ($LASTEXITCODE -ne 0) { throw "VICE ARM32 build failed for $EmuType." }

$built = Join-Path $third ("vice_" + $EmuType + "_libretro.dll")
if (!(Test-Path $built)) { throw "VICE build reported success but $built was not produced." }

Write-Host "Built: $built"
