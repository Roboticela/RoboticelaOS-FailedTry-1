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
$files = @(
    "EFI\BOOT\BOOTX64.EFI",
    "RoboKernel.elf",
    "DESKTOP.BIN",
    "RoboApp.bin",
    "INTERR.TTF",
    "INTERM.TTF",
    "INTERS.TTF",
    "INTERB.TTF",
    "INTERI.TTF",
    "ROBOLOGO.SVG",
    "EMPTYFOL.SVG",
    "BACKGROU.RJP",
    "BACKGROU.MP4"
)
foreach ($relative in $files) {
    $source = Join-Path $esp $relative
    if (-not (Test-Path $source)) {
        Write-Error "Build\Esp is missing $relative. Run .\RoboBuild.ps1 first."
    }
    $target = Join-Path $destination $relative
    $parent = Split-Path -Parent $target
    if (-not (Test-Path $parent)) {
        New-Item -ItemType Directory -Force -Path $parent | Out-Null
    }
    Copy-Item -Force $source $target
}
Write-Host "Copied the bootloader, kernel, desktop, fonts, and background to $destination"
