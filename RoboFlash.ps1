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
if (-not (Test-Path $esp)) {
    Write-Error "Build\Esp is missing. Run .\RoboBuild.ps1 first."
}

# Required first so a large optional file cannot block fonts and wallpaper.
$required = @(
    "EFI\BOOT\BOOTX64.EFI",
    "RoboKernel.elf",
    "INTERR.TTF",
    "INTERM.TTF",
    "INTERS.TTF",
    "INTERB.TTF",
    "INTERI.TTF",
    "ROBOLOGO.SVG",
    "EMPTYFOL.SVG",
    "BACKGROU.RJP",
    "DESKTOP.BIN",
    "RoboApp.bin"
)
$optional = @(
    "DevKit.elf",
    "BACKGROU.MP4"
)

function Copy-RoboEspFile([string]$Relative, [bool]$Required) {
    $source = Join-Path $esp $Relative
    if (-not (Test-Path $source)) {
        if ($Required) {
            Write-Error "Build\Esp is missing $Relative. Run .\RoboBuild.ps1 first."
        }
        Write-Host "skip  $Relative (not in Build\Esp)"
        return
    }
    $target = Join-Path $destination $Relative
    $parent = Split-Path -Parent $target
    if (-not [string]::IsNullOrWhiteSpace($parent) -and -not (Test-Path $parent)) {
        New-Item -ItemType Directory -Force -Path $parent | Out-Null
    }
    $sourceItem = Get-Item -LiteralPath $source
    Write-Host ("copy  {0} ({1:N0} bytes)" -f $Relative, $sourceItem.Length)
    Copy-Item -LiteralPath $source -Destination $target -Force
    if (-not (Test-Path -LiteralPath $target)) {
        Write-Error "Failed to copy $Relative to $destination"
    }
    $targetItem = Get-Item -LiteralPath $target
    if ($targetItem.Length -ne $sourceItem.Length) {
        Write-Error ("Size mismatch for {0}: source {1} bytes, drive {2} bytes" -f $Relative, $sourceItem.Length, $targetItem.Length)
    }
}

foreach ($relative in $required) {
    Copy-RoboEspFile $relative $true
}
foreach ($relative in $optional) {
    Copy-RoboEspFile $relative $false
}

try {
    Write-VolumeCache -DriveLetter $letter -ErrorAction Stop
} catch {
    try {
        $volume = Get-Volume -DriveLetter $letter -ErrorAction Stop
        if ($null -ne $volume) {
            Write-Host "Flush the USB safely before unplugging (write cache may still be pending)."
        }
    } catch {
    }
}

Write-Host ""
Write-Host "Copied bootloader, kernel, fonts, logos, and background to $destination"
Write-Host "Eject the drive safely before moving it to the test machine."
