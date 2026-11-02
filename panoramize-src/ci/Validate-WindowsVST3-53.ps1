# Windows x64 acceptance gate: fail closed. This script must actually run on Windows.
[CmdletBinding()]
param([string]$BuildRoot='build-win', [Parameter(Mandatory=$true)][string]$PluginvalExe)
$ErrorActionPreference='Stop'
$root=(Resolve-Path $BuildRoot).Path
$exe=(Resolve-Path $PluginvalExe).Path
$bundleFolder=Join-Path $root 'Panoramize_Plugin_artefacts/Release/VST3'
if (-not (Test-Path $bundleFolder)) { throw 'VST3 output missing; Windows build did not produce it' }
$bundles=@(Get-ChildItem $bundleFolder -Directory -Filter '*.vst3')
if ($bundles.Count -ne 1) { throw "Expected one VST3 bundle, got $($bundles.Count)" }
$bundle=$bundles[0].FullName
$dlls=@(Get-ChildItem (Join-Path $bundle 'Contents/x86_64-win') -File -Filter '*.vst3' -ErrorAction SilentlyContinue)
if ($dlls.Count -ne 1 -or $dlls[0].Length -lt 10000) { throw 'Missing or abnormally small VST3 DLL' }
$logDir=Join-Path $root 'validation-logs'
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$record=Join-Path $logDir 'windows_validation53.json'
# Initially record failure; a missing process report must never become a pass.
$report=[ordered]@{timestamp_utc=(Get-Date).ToUniversalTime().ToString('o');platform='windows-x64';
 bundle=$bundle;dll_size=$dlls[0].Length;pluginval_passed=$false;studio_one_manually_verified=$false;
 stage='starting';error='not finished'}
$report|ConvertTo-Json -Depth 6|Set-Content $record -Encoding UTF8
try {
 $stdout=Join-Path $logDir 'pluginval-stdout.txt'
 $stderr=Join-Path $logDir 'pluginval-stderr.txt'
 # GUI-inclusive and in-process for deterministic CI. The selected binary's
 # real ExitCode is authoritative; stdout/stderr are retained for diagnosis.
 $args=@('--strictness-level','5','--timeout-ms','120000',
  '--random-seed','12345678','--validate-in-process',
  '--output-dir',$logDir,'--validate',$bundle)
 $proc=Start-Process -FilePath $exe -ArgumentList $args -Wait -PassThru -NoNewWindow `
    -RedirectStandardOutput $stdout -RedirectStandardError $stderr
 if ($null -eq $proc.ExitCode -or $proc.ExitCode -ne 0) {
  throw "pluginval strictness5 failed: exit=$($proc.ExitCode). Logs: $logDir"
 }
 $report.pluginval_passed=$true
 $report.stage='completed_pluginval'
 $report.error=''
 $report|ConvertTo-Json -Depth 6|Set-Content $record -Encoding UTF8
 Write-Host "Windows VST3 + pluginval gate passed; saved report $record"
 Write-Warning 'Studio One itself has NOT been launched by this script.'
} catch {
 $report.stage='failed'
 $report.error=$_.Exception.Message
 $report|ConvertTo-Json -Depth 6|Set-Content $record -Encoding UTF8
 throw
}
