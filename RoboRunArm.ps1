$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot
Set-Location $Root

$esp = Join-Path $Root "Build\EspArm"
$bootloader = Join-Path $esp "EFI\BOOT\BOOTAA64.EFI"
$kernel = Join-Path $esp "RoboKernel.elf"
if (-not (Test-Path $bootloader) -or -not (Test-Path $kernel)) {
    Write-Error "The ARM64 ESP image is missing. Run .\RoboBuild.ps1 first."
}

function Find-Tool([string]$Name) {
    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }
    return $null
}

$qemu = Find-Tool "qemu-system-aarch64"
if (-not $qemu) {
    Write-Error "qemu-system-aarch64 was not found. Install QEMU and reopen the shell so it is on PATH."
}

$qemuDir = Split-Path -Parent $qemu
$share = Join-Path (Split-Path -Parent $qemuDir) "share"
if (-not (Test-Path $share)) {
    $share = Join-Path $qemuDir "share"
}

$code = Join-Path $share "edk2-aarch64-code.fd"
if (-not (Test-Path $code)) {
    Write-Error "edk2-aarch64-code.fd was not found next to QEMU."
}

$firmwareDir = Join-Path $Root "Build\OVMF"
New-Item -ItemType Directory -Force -Path $firmwareDir | Out-Null
$firmware = Join-Path $firmwareDir "edk2-aarch64-code.fd"
Copy-Item -Force $code $firmware
$vars = Join-Path $firmwareDir "edk2-aarch64-vars.fd"
if (-not (Test-Path $vars) -or (Get-Item $vars).Length -ne 64MB) {
    $empty = New-Object byte[] (64MB)
    [IO.File]::WriteAllBytes($vars, $empty)
}

$firmwarePath = $firmware -replace '\\', '/'
$varsPath = $vars -replace '\\', '/'
$espPath = $esp -replace '\\', '/'
$disk = Join-Path $Root "Build\RoboDisk.img"
$diskPath = $disk -replace '\\', '/'
$arguments = @(
    "-machine", "virt,gic-version=2",
    "-cpu", "cortex-a57",
    "-m", "512",
    "-smp", "1",
    "-net", "none",
    "-device", "ramfb",
    "-drive", "if=pflash,format=raw,readonly=on,file=$firmwarePath",
    "-drive", "if=pflash,format=raw,file=$varsPath",
    "-drive", "if=none,id=esp,format=raw,file=fat:rw:$espPath",
    "-device", "virtio-blk-pci,drive=esp"
)
if (Test-Path $disk) {
    $arguments += @(
        "-drive", "if=none,id=robo,format=raw,file=$diskPath",
        "-device", "virtio-blk-pci,drive=robo"
    )
}

& $qemu @arguments
exit $LASTEXITCODE
