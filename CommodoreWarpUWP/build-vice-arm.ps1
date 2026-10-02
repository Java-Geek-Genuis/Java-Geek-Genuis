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

if ($makefile -notmatch "CommodoreWarpUWP ARM32 MSVC") {
    $uwpBlock = @'
# CommodoreWarpUWP ARM32 MSVC
else ifneq (,$(findstring windows_msvc2017,$(platform)))
    NO_GCC := 1
    WINDOWS_VERSION = 1
    PlatformSuffix = $(subst windows_msvc2017_,,$(platform))
    ifneq (,$(findstring uwp,$(PlatformSuffix)))
        WinPartition = uwp
        MSVC2017CompileFlags = -DWINAPI_FAMILY=WINAPI_FAMILY_APP -D_WINDLL -D_UNICODE -DUNICODE -D__WRL_NO_DEFAULT_LIB__ -D_CRT_SECURE_NO_WARNINGS -EHsc -FS
        LDFLAGS += -APPCONTAINER -NXCOMPAT -DYNAMICBASE -MANIFEST:NO -OPT:REF -SUBSYSTEM:CONSOLE -MANIFESTUAC:NO -OPT:ICF -ERRORREPORT:PROMPT -NOLOGO -TLBID:1 -DEBUG:FULL -WINMD:NO
        LDFLAGS += WindowsApp.lib
    endif
    CFLAGS += $(MSVC2017CompileFlags)
    CXXFLAGS += $(MSVC2017CompileFlags)
    TargetArchMoniker = $(subst $(WinPartition)_,,$(PlatformSuffix))
    CC = cl.exe
    CXX = cl.exe
    LD = link.exe
    fpic :=
endif
'@
    $makefile = $makefile.Replace("# Wincross64", $uwpBlock + [Environment]::NewLine + "# Wincross64")
}

$makefile = $makefile.Replace(
    'COMMONFLAGS += -O3 -DNDEBUG -Wno-format -Wno-format-security',
    'ifneq (,$(findstring msvc,$(platform)))' + [Environment]::NewLine + '   COMMONFLAGS += -DNDEBUG' + [Environment]::NewLine + 'else' + [Environment]::NewLine + '   COMMONFLAGS += -O3 -DNDEBUG -Wno-format -Wno-format-security' + [Environment]::NewLine + 'endif'
)
$makefile = $makefile.Replace(
    'LDFLAGS     += -s',
    'ifneq (,$(findstring msvc,$(platform)))' + [Environment]::NewLine + 'else' + [Environment]::NewLine + '   LDFLAGS += -s' + [Environment]::NewLine + 'endif'
)
$makefile = $makefile.Replace(
    'CFLAGS      += $(fpic) $(INCFLAGS) $(COMMONFLAGS) -Wno-old-style-definition',
    'CFLAGS      += $(fpic) $(INCFLAGS) $(COMMONFLAGS)' + [Environment]::NewLine + 'ifneq (,$(findstring msvc,$(platform)))' + [Environment]::NewLine + 'else' + [Environment]::NewLine + '   CFLAGS += -Wno-old-style-definition' + [Environment]::NewLine + 'endif'
)
$makefile = $makefile.Replace(
    'LDFLAGS     += -lm $(fpic)',
    'ifneq (,$(findstring msvc,$(platform)))' + [Environment]::NewLine + 'else' + [Environment]::NewLine + '   LDFLAGS += -lm $(fpic)' + [Environment]::NewLine + 'endif'
)
$makefile = $makefile.Replace(
    'CXXFLAGS    += -std=c++98',
    'ifneq (,$(findstring msvc,$(platform)))' + [Environment]::NewLine + 'else' + [Environment]::NewLine + '   CXXFLAGS += -std=c++98' + [Environment]::NewLine + 'endif'
)
$makefile = $makefile.Replace(
    'COMMONFLAGS += -DHAVE_CONFIG_H -MMD -D__LIBRETRO__',
    'COMMONFLAGS += -DHAVE_CONFIG_H -D__LIBRETRO__' + [Environment]::NewLine + 'ifneq (,$(findstring msvc,$(platform)))' + [Environment]::NewLine + 'else' + [Environment]::NewLine + '   COMMONFLAGS += -MMD' + [Environment]::NewLine + 'endif'
)

$oldLink = @'
else
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)
endif
'@
$newLink = @'
else ifneq (,$(findstring msvc,$(platform)))
	$(CXX) -LD -Fe$@ $(OBJECTS) $(LDFLAGS)
else
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)
endif
'@
$makefile = $makefile.Replace($oldLink, $newLink)

$oldC = @'
	$(CC) $(CFLAGS) -c -o $@ $<
'@
$newC = @'
ifneq (,$(findstring msvc,$(platform)))
	$(CC) $(CFLAGS) -c -Fo$@ $<
else
	$(CC) $(CFLAGS) -c -o $@ $<
endif
'@
$makefile = $makefile.Replace($oldC, $newC)

$oldCpp = @'
	$(CXX) $(CXXFLAGS) -c -o $@ $<
'@
$newCpp = @'
ifneq (,$(findstring msvc,$(platform)))
	$(CXX) $(CXXFLAGS) -c -Fo$@ $<
else
	$(CXX) $(CXXFLAGS) -c -o $@ $<
endif
'@
$makefile = $makefile.Replace($oldCpp, $newCpp)

Set-Content ".\Makefile.uwp.arm32" $makefile -Encoding UTF8

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
