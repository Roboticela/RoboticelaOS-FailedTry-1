# Shared ESP asset lists for Flash, Run, and build asserts.
# Keep in sync with RoboBootloader load paths and RoboCompileDesktop staging.

$RoboEspRequired = @(
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
    "RoboApp.bin",
    "ROBOICON.PAK",
    "ROBOCTRY.PAK",
    "ROBOCAT.PAK",
    "ROBOPANE.PAK"
)

$RoboEspOptional = @(
    "DevKit.elf",
    "BACKGROU.MP4"
)

# Writable RoboFS volume. QEMU attaches it beside the ESP folder; Flash copies the same file.
$RoboDiskImage = "Build\RoboDisk.img"

$RoboEspArmRequired = @(
    "EFI\BOOT\BOOTAA64.EFI",
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
    "RoboApp.bin",
    "ROBOICON.PAK",
    "ROBOCTRY.PAK",
    "ROBOCAT.PAK",
    "ROBOPANE.PAK"
)

function Assert-RoboEspAssets {
    param(
        [Parameter(Mandatory = $true)]
        [string]$EspRoot,
        [string[]]$Required = $RoboEspRequired
    )
    foreach ($relative in $Required) {
        $path = Join-Path $EspRoot $relative
        if (-not (Test-Path -LiteralPath $path)) {
            Write-Error "ESP is missing $relative under $EspRoot. Run .\RoboBuild.ps1 first."
        }
    }
}
