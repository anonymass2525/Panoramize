# Panoramize 5.4 — BUILD + PLUGINVAL + OPTIONAL LOCAL INSTALLATION.
# This script is intended for Windows 11 x64 + Visual Studio 2022 with Desktop C++ and CMake.
# It does NOT launch or validate Studio One; a separate human acceptance test is required.
[CmdletBinding()]
param(
 [ValidateSet('None','User','Machine')][string]$InstallScope='None',
 [string]$JucePath='',
 [string]$PluginvalExe='',
 [string]$BuildRoot='build-win',
 [switch]$SkipTests
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$build = Join-Path $root $BuildRoot
$logs = Join-Path $root 'windows-validation54'
New-Item -Path $logs -ItemType Directory -Force | Out-Null
$reportPath = Join-Path $logs 'acceptance54.json'
$report = [ordered]@{
  utc = (Get-Date).ToUniversalTime().ToString('o')
  platform = [Environment]::OSVersion.VersionString
  cmake_configure = $false; msvc_build = $false; ctest = $false; pluginval_strictness5 = $false
  installed = $false; studio_one_manually_verified = $false; stage = 'not_started'
  bundle = ''; sha256 = ''; error = ''
}
function Save-Report { $report | ConvertTo-Json -Depth 5 | Set-Content $reportPath -Encoding UTF8 }
function Run-Checked([string]$label,[string]$command,[string[]]$arguments) {
  $report.stage = $label; Save-Report
  & $command @arguments *>&1 | Tee-Object -FilePath (Join-Path $logs "$label.log")
  if ($LASTEXITCODE -ne 0) { throw "$label FAILED: exit $LASTEXITCODE. See $logs" }
}
Save-Report
try {
  if ($env:OS -ne 'Windows_NT' -or -not [Environment]::Is64BitOperatingSystem) { throw 'Windows x64 is required' }
  $null = Get-Command cmake -ErrorAction Stop
  if ([string]::IsNullOrEmpty($JucePath)) {
    $JucePath = Join-Path $root 'external/JUCE'
    if (-not (Test-Path (Join-Path $JucePath 'modules/juce_audio_processors'))) {
      $null = Get-Command git -ErrorAction Stop
      New-Item (Split-Path $JucePath) -ItemType Directory -Force | Out-Null
      Run-Checked 'fetch_juce' 'git' @('clone','--filter=blob:none','--depth','1','--branch','8.0.16-security',
            'https://github.com/juce-framework/JUCE.git',$JucePath)
    }
  }
  $JucePath=(Resolve-Path $JucePath).Path
  if(-not (Test-Path (Join-Path $JucePath 'CMakeLists.txt'))) { throw 'JUCE CMakeLists.txt missing' }
  if([string]::IsNullOrEmpty($PluginvalExe)) {
    $pdir=Join-Path $root 'external/pluginval-1.0.4'
    $PluginvalExe=Join-Path $pdir 'pluginval.exe'
    if(-not (Test-Path $PluginvalExe)) {
      New-Item -ItemType Directory -Force -Path $pdir | Out-Null
      $archive=Join-Path $pdir 'pluginval_Windows.zip'
      Invoke-WebRequest -Uri 'https://github.com/Tracktion/pluginval/releases/download/v1.0.4/pluginval_Windows.zip' -OutFile $archive
      Expand-Archive -Path $archive -DestinationPath $pdir -Force
      $found=Get-ChildItem -Path $pdir -Filter 'pluginval.exe' -File -Recurse | Select-Object -First 1
      if(-not $found) { throw 'No pluginval.exe found inside official release archive' }
      $PluginvalExe=$found.FullName
    }
  }
  $PluginvalExe=(Resolve-Path $PluginvalExe).Path
  Run-Checked 'cmake_configure' 'cmake' @('-S',$root,'-B',$build,'-G','Visual Studio 17 2022','-A','x64',
        '-DPANORAMA_BUILD_VST3=ON','-DPANORAMA_BUILD_JUCE_SMOKE=ON',"-DPANORAMA_JUCE_PATH=$JucePath")
  $report.cmake_configure=$true;Save-Report
  Run-Checked 'msvc_release' 'cmake' @('--build',$build,'--config','Release','--parallel','4')
  $report.msvc_build=$true;Save-Report
  if(-not $SkipTests){
    Run-Checked 'ctest' 'ctest' @('--test-dir',$build,'-C','Release','--output-on-failure','--timeout','180',
    '-E','^(PANORAMA_DSP|Panoramize_DeepInverse46|Panoramize_Recorded51|Panoramize_Unified50)$')
    $report.ctest=$true;Save-Report
  }
  $folder=Join-Path $build 'Panoramize_Plugin_artefacts/Release/VST3'
  $bundles=@(Get-ChildItem -Path $folder -Directory -Filter '*.vst3' -ErrorAction Stop)
  if($bundles.Count -ne 1){ throw "Expected exactly one VST3 bundle, found $($bundles.Count)" }
  $bundle=$bundles[0].FullName
  $dlls=@(Get-ChildItem -Path (Join-Path $bundle 'Contents/x86_64-win') -Filter '*.vst3' -File -ErrorAction Stop)
  if($dlls.Count -ne 1 -or $dlls[0].Length -lt 10000){throw 'Native x64 VST3 binary missing or too small'}
  $report.bundle=$bundle; $report.sha256=(Get-FileHash $dlls[0].FullName -Algorithm SHA256).Hash;Save-Report
  Run-Checked 'pluginval5' $PluginvalExe @('--strictness-level','5','--timeout-ms','120000',
      '--random-seed','12345678','--validate-in-process','--output-dir',$logs,'--validate',$bundle)
  $report.pluginval_strictness5=$true;Save-Report
  if ($InstallScope -ne 'None') {
    $destRoot=if ($InstallScope -eq 'User') { Join-Path $env:LOCALAPPDATA 'Programs/Common/VST3' } else { Join-Path $env:CommonProgramFiles 'VST3' }
    New-Item -Path $destRoot -ItemType Directory -Force | Out-Null
    $target=Join-Path $destRoot $bundles[0].Name
    if(Test-Path $target) {
      $backup="$target.backup-$(Get-Date -Format 'yyyyMMdd-HHmmss')"
      Move-Item -LiteralPath $target -Destination $backup -ErrorAction Stop
      Write-Host "Previous plugin preserved: $backup"
    }
    Copy-Item -LiteralPath $bundle -Destination $target -Recurse -Force
    $report.installed=$true;Save-Report
    Write-Host "Installed to $target"
  }
  $report.stage='passed_windows_gate';Save-Report
  Write-Host 'PASS: Windows build and pluginval strictness 5. Studio One is NOT yet verified.'
  Write-Host "Logs: $logs"
} catch {
  $report.error=$_.Exception.Message;$report.stage='failed';Save-Report
  Write-Error $_.Exception.Message
  exit 1
}
