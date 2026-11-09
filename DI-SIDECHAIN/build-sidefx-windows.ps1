# Builds DI SIDEFX as a separate VST3 alongside DI SIDECHAIN.
# Run from the repository root:
# powershell -ExecutionPolicy Bypass -File .\DI-SIDECHAIN\build-sidefx-windows.ps1
[CmdletBinding()]
param(
  [string]$BuildDirectory = "",
  [switch]$SkipPackage,
  [string]$PluginvalExe = ""
)
$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$source = $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($BuildDirectory)) {
  $BuildDirectory = Join-Path (Split-Path $source -Parent) 'build\di-sidefx-local'
}
$BuildDirectory = [System.IO.Path]::GetFullPath($BuildDirectory)
$artifactDirectory = Join-Path $BuildDirectory 'DISidefx_artefacts\Release\VST3'
$packageDirectory = Join-Path $BuildDirectory 'packages'

function Run-Checked([string]$command, [string[]]$arguments) {
  & $command @arguments
  if ($LASTEXITCODE -ne 0) {
    throw "$command exited with code $LASTEXITCODE"
  }
}

Run-Checked 'cmake' @('-S', $source, '-B', $BuildDirectory,
  '-G', 'Visual Studio 17 2022', '-A', 'x64', '-DBUILD_TESTING=ON')
Run-Checked 'cmake' @('--build', $BuildDirectory, '--config', 'Release', '--parallel', '2')
Run-Checked 'ctest' @('--test-dir', $BuildDirectory, '-C', 'Release',
  '--output-on-failure', '--no-tests=error')

$bundles = @(Get-ChildItem -LiteralPath $artifactDirectory -Filter '*.vst3' -Directory)
if ($bundles.Count -ne 1) {
  throw "Expected exactly one DI SIDEFX VST3 bundle in $artifactDirectory, found $($bundles.Count)"
}
if ($bundles[0].Name -ne 'DI SIDEFX.vst3') {
  throw "Unexpected VST3 name: $($bundles[0].Name)"
}
if (-not [string]::IsNullOrWhiteSpace($PluginvalExe)) {
  if (-not (Test-Path -LiteralPath $PluginvalExe -PathType Leaf)) {
    throw "pluginval.exe not found at $PluginvalExe"
  }
  $pluginArgs = @('--strictness-level', '5', '--validate-in-process',
    '--timeout-ms', '120000', '--validate', ('"' + $bundles[0].FullName + '"'))
  $p = Start-Process -FilePath $PluginvalExe -ArgumentList $pluginArgs -Wait -PassThru -NoNewWindow
  if ($p.ExitCode -ne 0) { throw "pluginval failed with code $($p.ExitCode)" }
}

if (-not $SkipPackage) {
  New-Item -ItemType Directory -Force -Path $packageDirectory | Out-Null
  $zip = Join-Path $packageDirectory 'DI-SIDEFX-5.2B-Windows-x64.zip'
  if (Test-Path -LiteralPath $zip) { Remove-Item -LiteralPath $zip -Force }
  Compress-Archive -LiteralPath $bundles[0].FullName -DestinationPath $zip -CompressionLevel Optimal
  if (-not (Test-Path -LiteralPath $zip)) { throw "ZIP packaging failed" }
  Write-Host "ZIP: $zip"
}
Write-Host "PASS: DI SIDEFX VST3 built and CTest completed."
