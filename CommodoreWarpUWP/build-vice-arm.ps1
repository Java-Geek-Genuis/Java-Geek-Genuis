param([ValidateSet("x64","x128")][string]$EmuType="x64")
$ErrorActionPreference="Stop"
$root=Split-Path -Parent $MyInvocation.MyCommand.Path
$third=Join-Path $root "ThirdParty\vice-libretro"
if(!(Test-Path (Join-Path $third "Makefile"))){
  New-Item -ItemType Directory -Force (Split-Path $third) | Out-Null
  git clone https://github.com/libretro/vice-libretro.git $third
}
Set-Location $third
git fetch --depth 1 origin 9d7983826ea792f6cce7fdfe6c09488129c6f886
git checkout --detach 9d7983826ea792f6cce7fdfe6c09488129c6f886
make EMUTYPE=$EmuType platform=windows_msvc2017_uwp_arm
