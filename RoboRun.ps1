$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot
Set-Location $Root

$esp = Join-Path $Root "Build\Esp"
$bootloader = Join-Path $esp "EFI\BOOT\BOOTX64.EFI"
$kernel = Join-Path $esp "RoboKernel.elf"
if (-not (Test-Path $bootloader) -or -not (Test-Path $kernel)) {
    Write-Error "The ESP image is missing. Run .\RoboBuild.ps1 first."
}

function Find-Tool([string]$Name) {
    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }
    return $null
}

$qemu = Find-Tool "qemu-system-x86_64"
if (-not $qemu) {
    Write-Error "qemu-system-x86_64 was not found. Install QEMU and reopen the shell so it is on PATH."
}

$qemuDir = Split-Path -Parent $qemu
$share = Join-Path (Split-Path -Parent $qemuDir) "share"
if (-not (Test-Path $share)) {
    $share = Join-Path $qemuDir "share"
}

$codeNames = @(
    "edk2-x86_64-code.fd",
    "edk2-x86_64-secure-code.fd",
    "OVMF_CODE.fd",
    "OVMF.fd"
)
$varsNames = @(
    "edk2-x86_64-vars.fd",
    "OVMF_VARS.fd"
)
$searchDirs = @($share, (Join-Path $Root "Build\OVMF"), $qemuDir)

function Find-Firmware([string[]]$Names) {
    foreach ($dir in $searchDirs) {
        foreach ($name in $Names) {
            $path = Join-Path $dir $name
            if (Test-Path $path) {
                return $path
            }
        }
    }
    return $null
}

function Copy-Firmware([string]$Source, [string]$DestinationName) {
    $directory = Join-Path $Root "Build\OVMF"
    New-Item -ItemType Directory -Force -Path $directory | Out-Null
    $destination = Join-Path $directory $DestinationName
    Copy-Item -Force $Source $destination
    return ($destination -replace '\\', '/')
}

$code = Find-Firmware $codeNames
if (-not $code) {
    Write-Error "OVMF firmware was not found. Install QEMU's edk2 firmware, or place OVMF.fd in Build\OVMF."
}

$espPath = $esp -replace '\\', '/'
$arguments = @(
    "-machine", "pc",
    "-m", "256",
    "-smp", "1",
    "-net", "none",
    "-vga", "std",
    "-device", "qemu-xhci,id=xhci",
    "-drive", "if=ide,format=raw,file=fat:rw:$espPath"
)

$codeName = Split-Path -Leaf $code
if ($codeName -eq "OVMF.fd") {
    $bios = Copy-Firmware $code "OVMF.fd"
    $arguments += @("-bios", $bios)
} else {
    $firmware = Copy-Firmware $code "edk2-x86_64-code.fd"
    $arguments += @("-drive", "if=pflash,format=raw,readonly=on,file=$firmware")
    $vars = Find-Firmware $varsNames
    if ($vars) {
        $varsCopy = Copy-Firmware $vars "edk2-x86_64-vars.fd"
        $arguments += @("-drive", "if=pflash,format=raw,file=$varsCopy")
    }
}

& $qemu @arguments
exit $LASTEXITCODE
