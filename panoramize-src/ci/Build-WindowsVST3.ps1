# Panoramize 3.2.0 — Windows x64 release build and tests.
# Run from an ordinary PowerShell terminal with VS2022 C++ workload installed.
[CmdletBinding()]
param(
 [Parameter(Mandatory=$true)][string]$JucePath,
 [string]$PluginvalExe = '',
 [string]$BuildRoot = 'build-win'
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$juce = (Resolve-Path $JucePath).Path
if (-not (Test-Path (Join-Path $juce 'modules/juce_audio_processors'))) {
 throw 'The specified JUCE directory must contain modules/juce_audio_processors.'
}
$build = Join-Path $root $BuildRoot
Write-Host '[1/4] Configure Panoramize 3.2.0' -ForegroundColor Cyan
& cmake -S $root -B $build -G 'Visual Studio 17 2022' -A x64 '-DPANORAMA_BUILD_VST3=ON' '-DPANORAMA_BUILD_JUCE_SMOKE=ON' "-DPANORAMA_JUCE_PATH=$juce"
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed' }
Write-Host '[2/4] Build Release' -ForegroundColor Cyan
& cmake --build $build --config Release --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Release build failed' }
Write-Host '[3/4] Run DSP / JUCE smoke tests' -ForegroundColor Cyan
& ctest --test-dir $build -C Release --output-on-failure --timeout 180
if ($LASTEXITCODE -ne 0) { throw 'CTest failed' }
Write-Host '[4/4] Check bundle and optional pluginval' -ForegroundColor Cyan
$verify = Join-Path $PSScriptRoot 'Validate-WindowsVST3.ps1'
& $verify -BuildRoot $build -PluginvalExe $PluginvalExe
$pluginRoot = Join-Path $build 'Panoramize_Plugin_artefacts/Release/VST3'
$bundle = @(Get-ChildItem $pluginRoot -Directory -Filter '*.vst3') | Select-Object -First 1
$dest = Join-Path $root 'Panoramize_v3.2.0_Windows-x64-VST3.zip'
Compress-Archive -Path $bundle.FullName -DestinationPath $dest -Force
Write-Host "VST3 bundle compressed to $dest"
if (-not $PluginvalExe) {
 Write-Warning 'pluginval not run: specify -PluginvalExe for full GUI-inclusive validation.'
}
Write-Warning 'Studio One cannot be verified automatically; follow docs/STUDIO_ONE_V32_ACCEPTANCE_JA.md on the target PC.'
