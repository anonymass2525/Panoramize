# Run on a real Windows 11/Windows Server x64 system with Visual Studio 2022 C++.
# This performs an actual compile and pluginval, but not Studio One UI automation.
[CmdletBinding()]
param(
 [Parameter(Mandatory=$true)][string]$JucePath,
 [Parameter(Mandatory=$true)][string]$PluginvalExe,
 [string]$BuildRoot='build-win'
)
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$juce=(Resolve-Path $JucePath).Path
if (-not (Test-Path (Join-Path $juce 'modules/juce_audio_processors'))) {throw 'JUCE modules not found'}
$build=Join-Path $root $BuildRoot
& cmake -S $root -B $build -G 'Visual Studio 17 2022' -A x64 '-DPANORAMA_BUILD_VST3=ON' '-DPANORAMA_BUILD_JUCE_SMOKE=ON' "-DPANORAMA_JUCE_PATH=$juce"
if ($LASTEXITCODE -ne 0) {throw 'CMake configure FAILED'}
& cmake --build $build --config Release --parallel 4
if ($LASTEXITCODE -ne 0) {throw 'MSVC Release build FAILED'}
# Exclusions are explicit and also retained in the report. Unrun != pass.
$excluded='^(PANORAMA_DSP|Panoramize_DeepInverse46|Panoramize_Recorded51|Panoramize_Unified50)$'
& ctest --test-dir $build -C Release --output-on-failure --timeout 180 -E $excluded
if ($LASTEXITCODE -ne 0) {throw 'Windows CTest FAILED'}
& (Join-Path $PSScriptRoot 'Validate-WindowsVST3-53.ps1') -BuildRoot $build -PluginvalExe $PluginvalExe
if (-not $?) {throw 'pluginval verification FAILED'}
Write-Host 'Windows VST3 compile, selected CTest, and pluginval gate completed.'
Write-Warning 'Studio One interactive validation is SEPARATE and not automated here.'
