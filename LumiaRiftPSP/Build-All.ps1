param(
  [ValidateSet("Debug","Release")][string]$Configuration="Release",
  [string]$PspConfiguration="UWP Gold 14393",
  [switch]$SkipPsp
)
$ErrorActionPreference="Stop"
$root=Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

$pf86=[Environment]::GetFolderPath("ProgramFilesX86")
$vswhere=Join-Path $pf86 "Microsoft Visual Studio\Installer\vswhere.exe"
if(!(Test-Path $vswhere)){throw "Visual Studio 2022 was not found."}
$msbuild=& $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | Select-Object -First 1
if(!$msbuild){throw "MSBuild was not found."}

$stage=Join-Path $root "artifacts"
New-Item -ItemType Directory -Force $stage | Out-Null
Write-Host "Building Lumia Rift ARM32..."
& $msbuild (Join-Path $root "LumiaRiftGame.sln") /m /p:Configuration=$Configuration /p:Platform=ARM /p:AppxPackageSigningEnabled=false /p:AppxBundle=Always

Get-ChildItem $root -Recurse -Include *.appx,*.appxbundle,*.msixbundle -File |
  Where-Object {$_.FullName -notmatch "\ThirdParty\"} |
  ForEach-Object {Copy-Item $_.FullName $stage -Force}

if(!$SkipPsp){
  git submodule update --init --recursive
  $pp=Join-Path $root "ThirdParty\PPSSPP-UWP-ARM"
  $sln=Get-ChildItem $pp -Recurse -Filter PPSSPP_UWP.sln -File -ErrorAction SilentlyContinue | Select-Object -First 1
  if($sln){
    Write-Host "Building legacy PPSSPP ARM32..."
    & $msbuild $sln.FullName /m /p:Configuration="$PspConfiguration" /p:Platform=ARM /p:AppxPackageSigningEnabled=false
    Get-ChildItem $pp -Recurse -Include *.appx,*.appxbundle,*.msixbundle -File -ErrorAction SilentlyContinue |
      ForEach-Object {Copy-Item $_.FullName $stage -Force}
  }else{
    Write-Warning "PPSSPP_UWP.sln was not found in the initialized submodule."
  }
}else{
  Write-Host "Skipping PPSSPP build."
}

Get-ChildItem $stage -File | Select-Object Name,Length | Format-Table -AutoSize
