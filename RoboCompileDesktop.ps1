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

$clang = Find-Tool "clang++"
$ar = Find-Tool "llvm-ar"
if (-not $clang) {
    Write-Error "clang++ is required. Install LLVM and reopen the shell so it is on PATH."
}
if (-not $ar) {
    Write-Error "llvm-ar is required. Install LLVM and reopen the shell so it is on PATH."
}

$compiler = Join-Path $Root "Compiler"
$pageXml = Join-Path $Root "RoboDesktop\Desktop.xml"
$assets = Join-Path $compiler "Assets"
if (-not (Test-Path $pageXml)) {
    Write-Error "RoboDesktop\Desktop.xml is missing."
}

$toolDir = Join-Path $Root "Build\Desktop"
New-Item -ItemType Directory -Force -Path $toolDir | Out-Null
$toolExe = Join-Path $toolDir "XmlCompile.exe"
$generated = Join-Path $toolDir "DesktopPage.cpp"

$toolSources = @(
    (Join-Path $compiler "XMLCompiler\Source\XmlCompileMain.cpp"),
    (Join-Path $compiler "XMLCompiler\Source\XmlParser.cpp"),
    (Join-Path $compiler "XMLCompiler\Source\XmlToCpp.cpp"),
    (Join-Path $compiler "XMLCompiler\Source\SvgPath.cpp"),
    (Join-Path $compiler "XMLCompiler\Source\IconPack.cpp"),
    (Join-Path $compiler "Compiler\Source\Color.cpp"),
    (Join-Path $compiler "Compiler\Source\Foundation.cpp"),
    (Join-Path $compiler "Compiler\Source\RoboFoundation.cpp")
)

Write-Host "Compiling RoboDesktop\Desktop.xml for RoboticelaOS"
& $clang -std=c++17 -fexceptions -O2 `
    -I (Join-Path $compiler "XMLCompiler\Include") `
    -I (Join-Path $compiler "Compiler\Include") `
    -o $toolExe @toolSources
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& $toolExe $pageXml $generated $assets
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$devKitXml = Join-Path $compiler "DevKit\DevKit.xml"
$devKitGenerated = Join-Path $toolDir "DevKitPage.cpp"
if (-not (Test-Path $devKitXml)) {
    Write-Error "Compiler\DevKit\DevKit.xml is missing."
}
& $toolExe $devKitXml $devKitGenerated $assets
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
$devKitText = [IO.File]::ReadAllText($devKitGenerated)
$devKitText = $devKitText.Replace("void BuildPage(", "void BuildDevKitPage(")
[IO.File]::WriteAllText($devKitGenerated, $devKitText)

$sources = @(Get-ChildItem (Join-Path $compiler "Compiler\Source") -Filter *.cpp | ForEach-Object { $_.FullName })
$sources += @(
    $generated,
    $devKitGenerated,
    (Join-Path $compiler "RoboPlatforms\RoboticelaOS\RoboticelaOSPlatform.cpp")
)

$objects = @()
foreach ($source in $sources) {
    $object = Join-Path $toolDir (([IO.Path]::GetFileNameWithoutExtension($source)) + ".o")
    & $clang -std=c++17 -fexceptions -O2 `
        -I (Join-Path $compiler "Compiler\Include") `
        -I (Join-Path $compiler "Compiler\ThirdParty") `
        -I (Join-Path $compiler "XMLCompiler\Include") `
        -I (Join-Path $compiler "RoboPlatforms\RoboticelaOS") `
        -c $source -o $object
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    $objects += $object
}

$presentSource = Join-Path $compiler "RoboPlatforms\RoboticelaOS\RoboPresentDesktop.cpp"
$presentObject = Join-Path $toolDir "RoboPresentDesktop.o"
& $clang -std=c++17 -fexceptions -O2 `
    -I (Join-Path $compiler "Compiler\Include") `
    -I (Join-Path $compiler "Compiler\ThirdParty") `
    -I (Join-Path $compiler "RoboPlatforms\RoboticelaOS") `
    -I (Join-Path $Root "RoboInclude") `
    -c $presentSource -o $presentObject
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$library = Join-Path $toolDir "RoboticelaDesktop.lib"
if (Test-Path $library) {
    Remove-Item $library -Force
}
& $ar rcs $library @objects
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$presentExe = Join-Path $toolDir "RoboPresentDesktop.exe"
$linkList = Join-Path $toolDir "RoboPresentDesktop.link"
$linkLines = @($presentObject) + $objects | ForEach-Object { ($_ -replace '\\', '/') }
[IO.File]::WriteAllLines($linkList, $linkLines)
& $clang -o $presentExe ("@" + ($linkList -replace '\\', '/'))
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

. (Join-Path $Root "RoboEspAssets.ps1")
$esp = Join-Path $Root "Build\Esp"
$espArm = Join-Path $Root "Build\EspArm"
New-Item -ItemType Directory -Force -Path $esp, $espArm | Out-Null
$image = Join-Path $esp "DESKTOP.BIN"
& $presentExe $([string]1280) $([string]800) $image $assets
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
Copy-Item -Force $image (Join-Path $espArm "DESKTOP.BIN")
Copy-Item -Force (Join-Path $assets "Fonts\Inter-Regular.ttf") (Join-Path $esp "INTERR.TTF")
Copy-Item -Force (Join-Path $assets "Fonts\Inter-Medium.ttf") (Join-Path $esp "INTERM.TTF")
Copy-Item -Force (Join-Path $assets "Fonts\Inter-SemiBold.ttf") (Join-Path $esp "INTERS.TTF")
Copy-Item -Force (Join-Path $assets "Fonts\Inter-Bold.ttf") (Join-Path $esp "INTERB.TTF")
Copy-Item -Force (Join-Path $assets "Fonts\Inter-Italic.ttf") (Join-Path $esp "INTERI.TTF")
$logo = Join-Path $Root "Assets\RoboticelaLogo.svg"
if (Test-Path $logo) {
    Copy-Item -Force $logo (Join-Path $esp "ROBOLOGO.SVG")
    Copy-Item -Force $logo (Join-Path $espArm "ROBOLOGO.SVG")
}
$folderIcon = Join-Path $Root "Assets\EmptyFolder.svg"
if (Test-Path $folderIcon) {
    Copy-Item -Force $folderIcon (Join-Path $esp "EMPTYFOL.SVG")
    Copy-Item -Force $folderIcon (Join-Path $espArm "EMPTYFOL.SVG")
}
Copy-Item -Force (Join-Path $esp "INTERR.TTF") (Join-Path $espArm "INTERR.TTF")
Copy-Item -Force (Join-Path $esp "INTERM.TTF") (Join-Path $espArm "INTERM.TTF")
Copy-Item -Force (Join-Path $esp "INTERS.TTF") (Join-Path $espArm "INTERS.TTF")
Copy-Item -Force (Join-Path $esp "INTERB.TTF") (Join-Path $espArm "INTERB.TTF")
Copy-Item -Force (Join-Path $esp "INTERI.TTF") (Join-Path $espArm "INTERI.TTF")
$video = Join-Path $Root "Assets\Background.mp4"
if (Test-Path $video) {
    Copy-Item -Force $video (Join-Path $esp "BACKGROU.MP4")
    Copy-Item -Force $video (Join-Path $espArm "BACKGROU.MP4")
    & python (Join-Path $Root "RoboPackBackground.py")
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

# Stage list must stay aligned with RoboEspAssets.ps1 (fonts, logos, DESKTOP.BIN, BACKGROU.RJP).
$stagedRequired = @(
    "INTERR.TTF", "INTERM.TTF", "INTERS.TTF", "INTERB.TTF", "INTERI.TTF",
    "ROBOLOGO.SVG", "EMPTYFOL.SVG", "DESKTOP.BIN"
)
Assert-RoboEspAssets -EspRoot $esp -Required $stagedRequired
if (Test-Path (Join-Path $esp "BACKGROU.RJP")) {
    Assert-RoboEspAssets -EspRoot $esp -Required @("BACKGROU.RJP")
}

Write-Host "Built $generated"
Write-Host "Built $library"
Write-Host "Staged $image"
