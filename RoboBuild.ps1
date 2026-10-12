$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot
Set-Location $Root

function Find-Tool([string]$Name) {
    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }
    return $null
}

$clang = Find-Tool "clang"
$lld = Find-Tool "ld.lld"
$lldLink = Find-Tool "lld-link"
$make = Find-Tool "make"
if (-not $make) {
    $make = Find-Tool "mingw32-make"
}

if (-not $clang -or -not $lld -or -not $lldLink) {
    Write-Error "clang, ld.lld, and lld-link are required. Install LLVM and reopen the shell so they are on PATH."
}
if (-not $make) {
    Write-Error "GNU Make is required. Install make or mingw32-make and reopen the shell so it is on PATH."
}

& powershell -NoProfile -File (Join-Path $Root "RoboCompileDesktop.ps1")
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& $make
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

Write-Host "Staged Build\Esp\EFI\BOOT\BOOTX64.EFI and Build\EspArm\EFI\BOOT\BOOTAA64.EFI"
