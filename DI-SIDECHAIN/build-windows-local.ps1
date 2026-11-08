# DI SIDECHAIN 4 — local Windows Release build, without GitHub Actions minutes.
# Run from PowerShell: powershell -ExecutionPolicy Bypass -File .\DI-SIDECHAIN\build-windows-local.ps1
[CmdletBinding()]
param(
    [string]$BuildDirectory = "",
    [switch]$SkipPackage
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$source = $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($BuildDirectory)) {
    $BuildDirectory = Join-Path (Split-Path $source -Parent) 'build\di-sidechain-local'
}
$BuildDirectory = [System.IO.Path]::GetFullPath($BuildDirectory)
$packageDirectory = Join-Path $BuildDirectory 'packages'
$artifactDirectory = Join-Path $BuildDirectory 'DISidechain_artefacts\Release\VST3'

function Run-Checked([string]$command, [string[]]$arguments) {
    Write-Host ">> $command $($arguments -join ' ')"
    & $command @arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$command failed with exit code $LASTEXITCODE"
    }
}

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw 'CMake not found. Install CMake 3.22+ and Visual Studio 2022 C++ tools.'
}
if (-not (Get-Command ctest -ErrorAction SilentlyContinue)) {
    throw 'CTest not found. Install CMake with CTest support.'
}

Run-Checked 'cmake' @('-S', $source, '-B', $BuildDirectory,
    '-G', 'Visual Studio 17 2022', '-A', 'x64', '-DBUILD_TESTING=ON')
Run-Checked 'cmake' @('--build', $BuildDirectory, '--config', 'Release', '--parallel', '2')
Run-Checked 'ctest' @('--test-dir', $BuildDirectory, '-C', 'Release',
    '--output-on-failure', '--no-tests=error')

$tests = (& ctest --test-dir $BuildDirectory -C Release -N) -join "`n"
if ($LASTEXITCODE -ne 0) { throw 'Unable to enumerate CTest tests.' }
$testCount = ([regex]::Matches($tests, '(?m)^\s*Test\s*#\d+:')).Count
if ($testCount -ne 6) {
    throw "Expected exactly six DSP regression tests, found $testCount."
}

if (-not (Test-Path -LiteralPath $artifactDirectory -PathType Container)) {
    throw "VST3 artifact directory missing: $artifactDirectory"
}
$bundles = @(Get-ChildItem -LiteralPath $artifactDirectory -Filter '*.vst3' -Directory)
if ($bundles.Count -ne 1) {
    throw "Expected exactly one VST3 bundle; found $($bundles.Count) in $artifactDirectory"
}

Write-Host "PASS: six CTest tests and VST3 bundle $($bundles[0].Name)" -ForegroundColor Green
if (-not $SkipPackage) {
    New-Item -ItemType Directory -Force -Path $packageDirectory | Out-Null
    $zip = Join-Path $packageDirectory 'DI-SIDECHAIN-4-Windows-VST3.zip'
    if (Test-Path -LiteralPath $zip) { Remove-Item -LiteralPath $zip -Force }
    Compress-Archive -LiteralPath $bundles[0].FullName -DestinationPath $zip -CompressionLevel Optimal
    Write-Host "ZIP: $zip" -ForegroundColor Green
}
