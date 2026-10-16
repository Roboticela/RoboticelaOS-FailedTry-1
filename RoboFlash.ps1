param(
    [Parameter(Mandatory = $true)]
    [string]$Drive
)

$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot
Set-Location $Root
. (Join-Path $Root "RoboEspAssets.ps1")

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

Assert-RoboEspAssets -EspRoot $esp -Required $RoboEspRequired

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

foreach ($relative in $RoboEspRequired) {
    Copy-RoboEspFile $relative $true
}
foreach ($relative in $RoboEspOptional) {
    Copy-RoboEspFile $relative $false
}

$disk = Join-Path $Root "Build\RoboDisk.img"
if (Test-Path -LiteralPath $disk) {
    $diskTarget = Join-Path $destination "RoboDisk.img"
    $diskItem = Get-Item -LiteralPath $disk
    Write-Host ("copy  RoboDisk.img ({0:N0} bytes) — RoboFS volume for apps" -f $diskItem.Length)
    Copy-Item -LiteralPath $disk -Destination $diskTarget -Force
} else {
    Write-Host "note  Build\RoboDisk.img missing; USB is ESP-only (QEMU can still attach the disk image when present)."
}

Write-Host ""
Write-Host "Verifying files on ${letter}:"
foreach ($relative in ($RoboEspRequired + $RoboEspOptional)) {
    $target = Join-Path $destination $relative
    if (Test-Path -LiteralPath $target) {
        $item = Get-Item -LiteralPath $target
        Write-Host ("  ok    {0} ({1:N0} bytes)" -f $relative, $item.Length)
    } elseif ($RoboEspRequired -contains $relative) {
        Write-Error "Missing required file on drive after copy: $relative"
    } else {
        Write-Host "  skip  $relative"
    }
}

try {
    Write-VolumeCache -DriveLetter $letter -ErrorAction Stop
    Write-Host "Write cache flushed."
} catch {
    Write-Host "Flush the USB with 'Safely Remove Hardware' before unplugging."
}

Write-Host ""
Write-Host "Copied bootloader, kernel, fonts, logos, and background to $destination"
Write-Host "Eject the drive safely before moving it to the test machine."
