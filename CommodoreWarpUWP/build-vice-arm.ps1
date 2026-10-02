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
git fetch --depth 1 origin $pin
git checkout --detach $pin

$makefile = Get-Content ".\Makefile" -Raw

# Add a complete MSVC/UWP ARM32 platform branch to the existing VICE
# platform-selection chain.  Do not touch the surrounding else/endif
# structure except for inserting this single branch.
if ($makefile -notmatch "CommodoreWarpUWP ARM32 MSVC") {
    $uwpBlock = @'
# CommodoreWarpUWP ARM32 MSVC
else ifneq (,$(findstring windows_msvc2017_uwp_arm,$(platform)))
    NO_GCC := 1
    WINDOWS_VERSION = 1
    WinPartition = uwp
    TargetArchMoniker = arm
    MSVC2017CompileFlags = -DWINAPI_FAMILY=WINAPI_FAMILY_APP -D_WINDLL -D_UNICODE -DUNICODE -D__WRL_NO_DEFAULT_LIB__ -D_CRT_SECURE_NO_WARNINGS -EHsc -FS
    CFLAGS += $(MSVC2017CompileFlags)
    CXXFLAGS += $(MSVC2017CompileFlags)
    CFLAGS += -D__WIN32__
    CXXFLAGS += -D__WIN32__
    CXX = cl.exe
    CC = cl.exe
    LD = cl.exe
    fpic :=
'@
    $makefile = $makefile.Replace("# Wincross64", $uwpBlock + [Environment]::NewLine + "# Wincross64")
}

# Remove GCC-only command-line switches after the whole makefile has been
# assembled.  filter-out is safe because these are individual make words.
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
    $makefile = $makefile.Replace("# webOS", $compat + [Environment]::NewLine + "# webOS")
}

# Keep all recipes unconditional.  Only the output switches vary by platform.
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

Set-Content ".\Makefile.uwp.arm32" $makefile -Encoding UTF8

# Show the generated makefile area around the custom platform so a failed
# build has useful diagnostics in Actions logs.
$lineNumber = 0
Get-Content ".\Makefile.uwp.arm32" | ForEach-Object {
    $lineNumber++
    if ($lineNumber -ge 75 -and $lineNumber -le 125) {
        "{0,4}: {1}" -f $lineNumber, $_
    }
}

$vcvars = Join-Path "$env:ProgramFiles" "Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
if (!(Test-Path $vcvars)) {
    $vcvars = Join-Path "$env:ProgramFiles(x86)" "Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
}
if (!(Test-Path $vcvars)) { throw "Visual Studio vcvarsall.bat was not found." }

$make = (Get-Command make.exe -ErrorAction SilentlyContinue).Source
if (!$make) { $make = "C:\msys64\usr\bin\make.exe" }
if (!(Test-Path $make)) { throw "MSYS2 make.exe was not found." }

$cmd = 'call "' + $vcvars + '" x64_arm && set "PATH=C:\msys64\usr\bin;%PATH%" && "' + $make + '" -f Makefile.uwp.arm32 clean && "' + $make + '" -f Makefile.uwp.arm32 EMUTYPE=' + $EmuType + ' platform=windows_msvc2017_uwp_arm'
cmd.exe /d /s /c $cmd
if ($LASTEXITCODE -ne 0) { throw "VICE ARM32 build failed for $EmuType." }

$built = Join-Path $third ("vice_" + $EmuType + "_libretro.dll")
if (!(Test-Path $built)) { throw "VICE build reported success but $built was not produced." }

Write-Host "Built: $built"
