param(
    [Parameter(Mandatory = $true)]
    [string]$Drive
)

$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot
Set-Location $Root

if ($Drive -notmatch '^[A-Za-z]:?$') {
    Write-Error "Pass a drive letter, for example -Drive E"
}

$letter = $Drive.TrimEnd(':').ToUpperInvariant()
if ($letter -eq "C") {
    Write-Error "Refusing to copy the image onto C:."
}

$destination = "${letter}:\"
if (-not (Test-Path $destination)) {
    Write-Error "Drive $destination is not available. Format it as FAT32 first, then pass its letter."
}

$esp = Join-Path $Root "Build\Esp"
$bootloader = Join-Path $esp "EFI\BOOT\BOOTX64.EFI"
$kernel = Join-Path $esp "RoboKernel.elf"
if (-not (Test-Path $bootloader) -or -not (Test-Path $kernel)) {
    Write-Error "The ESP image is missing. Run .\RoboBuild.ps1 first."
}

Copy-Item -Force $kernel (Join-Path $destination "RoboKernel.elf")
$bootDir = Join-Path $destination "EFI\BOOT"
New-Item -ItemType Directory -Force -Path $bootDir | Out-Null
Copy-Item -Force $bootloader (Join-Path $bootDir "BOOTX64.EFI")
Write-Host "Copied the bootloader and kernel to $destination"
