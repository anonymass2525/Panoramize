# Validate-WindowsVST3.ps1
# Locate the actual x64 VST3 bundle; never claim success for a missing DLL.
[CmdletBinding()]
param(
 [string]$BuildRoot = 'build-win',
 [string]$PluginvalExe = ''
)
$ErrorActionPreference = 'Stop'
$base = (Resolve-Path $BuildRoot).Path
$pluginRoot = Join-Path $base 'Panoramize_Plugin_artefacts/Release/VST3'
if (-not (Test-Path $pluginRoot)) { throw "VST3 output folder missing: $pluginRoot" }
$bundles = @(Get-ChildItem $pluginRoot -Directory -Filter '*.vst3')
if ($bundles.Count -ne 1) { throw "Expected exactly one VST3 bundle; found $($bundles.Count)" }
$bundle = $bundles[0].FullName
$binaryDir = Join-Path $bundle 'Contents/x86_64-win'
$binary = @(Get-ChildItem $binaryDir -Filter '*.vst3' -File -ErrorAction SilentlyContinue)
if ($binary.Count -ne 1 -or $binary[0].Length -lt 10000) {
 throw "Missing or suspiciously small x64 VST3 DLL in $binaryDir"
}
Write-Host "VST3 bundle: $bundle"
Write-Host "VST3 DLL: $($binary[0].FullName) ($($binary[0].Length) bytes)"
if ($PluginvalExe) {
 $exe = (Resolve-Path $PluginvalExe).Path
 $logs = Join-Path $base 'validation-logs'
 New-Item -ItemType Directory -Force -Path $logs | Out-Null
 $args = @('--strictness-level','5','--timeout-ms','120000',
   '--random-seed','12345678','--validate-in-process',
   '--output-dir', ('"' + $logs + '"'),'--validate', ('"' + $bundle + '"'))
 Write-Host "Running pluginval with GUI tests enabled."
 # Start-Process -Wait avoids premature return from a GUI-subsystem executable.
 $proc = Start-Process -FilePath $exe -ArgumentList $args -Wait -PassThru -NoNewWindow
 # GUI-subsystem pluginval writes SUCCESS to its console in v1.0.4.
 # Do not assume an ALL TESTS PASSED string is present in its --output-dir files.
 # For Start-Process, the authoritative status is the returned process ExitCode,
 # NOT $LASTEXITCODE (which only tracks directly invoked native commands).
 if ($null -eq $proc.ExitCode) { throw 'pluginval returned no process exit code' }
 if ($proc.ExitCode -ne 0) { throw "pluginval failed, exit $($proc.ExitCode)" }
 Write-Host 'pluginval strictness 5 finished successfully (exit code 0).'
}
