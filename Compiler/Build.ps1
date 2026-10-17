param(
    [ValidateSet("Demo", "Desktop")]
    [string]$App = "Demo"
)

$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildWeb = Join-Path $Root "Build\Web"
$BuildWindows = Join-Path $Root "Build\Windows"
$EmsdkDir = Join-Path $Root "Build\emsdk"

function Write-Step([string]$Message) {
    Write-Host ""
    Write-Host "==> $Message" -ForegroundColor Cyan
}

function Ensure-Directory([string]$Path) {
    if (-not (Test-Path $Path)) {
        New-Item -ItemType Directory -Path $Path | Out-Null
    }
}

function Get-CoreSources {
    return @(
        (Join-Path $Root "Compiler\Source\Color.cpp"),
        (Join-Path $Root "Compiler\Source\TextAlign.cpp"),
        (Join-Path $Root "Compiler\Source\FontStyle.cpp"),
        (Join-Path $Root "Compiler\Source\Text.cpp"),
        (Join-Path $Root "Compiler\Source\Heading.cpp"),
        (Join-Path $Root "Compiler\Source\Paragraph.cpp"),
        (Join-Path $Root "Compiler\Source\Button.cpp"),
        (Join-Path $Root "Compiler\Source\Input.cpp"),
        (Join-Path $Root "Compiler\Source\Scrollbar.cpp"),
        (Join-Path $Root "Compiler\Source\Flexbox.cpp"),
        (Join-Path $Root "Compiler\Source\Grid.cpp"),
        (Join-Path $Root "Compiler\Source\Position.cpp"),
        (Join-Path $Root "Compiler\Source\Container.cpp"),
        (Join-Path $Root "Compiler\Source\Parse.cpp"),
        (Join-Path $Root "Compiler\Source\TextSelection.cpp"),
        (Join-Path $Root "Compiler\Source\FrameBuffer.cpp"),
        (Join-Path $Root "Compiler\Source\FontEngine.cpp"),
        (Join-Path $Root "Compiler\Source\TextPainter.cpp"),
        (Join-Path $Root "Compiler\Source\Svg.cpp"),
        (Join-Path $Root "Compiler\Source\Icon.cpp"),
        (Join-Path $Root "Compiler\Source\RoboIconPack.cpp"),
        (Join-Path $Root "Compiler\Source\RoboCatalog.cpp"),
        (Join-Path $Root "Compiler\Source\RoboPanePack.cpp"),
        (Join-Path $Root "Compiler\Source\RoboPageTables.cpp"),
        (Join-Path $Root "Compiler\Source\SvgPainter.cpp"),
        (Join-Path $Root "Compiler\Source\BrandLogo.cpp"),
        (Join-Path $Root "Compiler\Source\BrandLogoPainter.cpp"),
        (Join-Path $Root "Compiler\Source\TitleBar.cpp"),
        (Join-Path $Root "Compiler\Source\Desktop.cpp"),
        (Join-Path $Root "Compiler\Source\TopBarMenu.cpp"),
        (Join-Path $Root "Compiler\Source\ContextMenu.cpp"),
        (Join-Path $Root "Compiler\Source\ContextMenuPainter.cpp"),
        (Join-Path $Root "Compiler\Source\SelectionArea.cpp"),
        (Join-Path $Root "Compiler\Source\SelectionAreaPainter.cpp"),
        (Join-Path $Root "Compiler\Source\Background.cpp"),
        (Join-Path $Root "Compiler\Source\BackgroundImage.cpp"),
        (Join-Path $Root "Compiler\Source\BackgroundPainter.cpp"),
        (Join-Path $Root "Compiler\Source\DesktopPainter.cpp"),
        (Join-Path $Root "Compiler\Source\TitleBarPainter.cpp"),
        (Join-Path $Root "Compiler\Source\Sidebar.cpp"),
        (Join-Path $Root "Compiler\Source\SidebarPainter.cpp"),
        (Join-Path $Root "Compiler\Source\Separator.cpp"),
        (Join-Path $Root "Compiler\Source\SeparatorPainter.cpp"),
        (Join-Path $Root "Compiler\Source\ThemeToggle.cpp"),
        (Join-Path $Root "Compiler\Source\ThemeTogglePainter.cpp"),
        (Join-Path $Root "Compiler\Source\ThemeIconToggle.cpp"),
        (Join-Path $Root "Compiler\Source\ThemeIconTogglePainter.cpp"),
        (Join-Path $Root "Compiler\Source\Motion.cpp"),
        (Join-Path $Root "Compiler\Source\MotionShape.cpp"),
        (Join-Path $Root "Compiler\Source\Foundation.cpp"),
        (Join-Path $Root "Compiler\Source\RoboFoundation.cpp"),
        (Join-Path $Root "Compiler\Source\ButtonPainter.cpp"),
        (Join-Path $Root "Compiler\Source\InputPainter.cpp"),
        (Join-Path $Root "Compiler\Source\ScrollbarPainter.cpp"),
        (Join-Path $Root "Compiler\Source\FlexboxPainter.cpp"),
        (Join-Path $Root "Compiler\Source\GridPainter.cpp"),
        (Join-Path $Root "Compiler\Source\PositionPainter.cpp"),
        (Join-Path $Root "Compiler\Source\ContainerPainter.cpp"),
        (Join-Path $Root "Compiler\Source\RoboScene.cpp")
    )
}

function Get-RoboticelaOSPlatformSources {
    return @(
        (Join-Path $Root "RoboPlatforms\RoboticelaOS\RoboticelaOSPlatform.cpp")
    )
}

function Get-IncludeArgs([string]$CompilerStyle) {
    $paths = @(
        (Join-Path $Root "Compiler\Include"),
        (Join-Path $Root "Compiler\ThirdParty"),
        (Join-Path $Root "XMLCompiler\Include")
    )
    if ($CompilerStyle -eq "em") {
        return @($paths | ForEach-Object { "-I$_" })
    }
    return @($paths | ForEach-Object { "/I$_" })
}

function Find-Emcc {
    $existing = Get-Command emcc -ErrorAction SilentlyContinue
    if ($existing) {
        return $existing.Source
    }
    $candidate = Join-Path $EmsdkDir "upstream\emscripten\emcc.bat"
    if (Test-Path $candidate) {
        return $candidate
    }
    return $null
}

function Ensure-Emsdk {
    $activate = Join-Path $EmsdkDir "emsdk_env.ps1"
    if (Test-Path $activate) {
        & $activate | Out-Null
    }

    $emccPath = Find-Emcc
    if ($emccPath) {
        Write-Host "Using emcc: $emccPath"
        return
    }

    Write-Step "Emscripten not found. Installing emsdk into Build\emsdk (one-time)"
    Ensure-Directory (Join-Path $Root "Build")
    if (-not (Test-Path $EmsdkDir)) {
        git clone https://github.com/emscripten-core/emsdk.git $EmsdkDir
    }

    Push-Location $EmsdkDir
    try {
        & .\emsdk.bat install latest
        & .\emsdk.bat activate latest
        & .\emsdk_env.ps1 | Out-Null
    }
    finally {
        Pop-Location
    }

    $emccPath = Find-Emcc
    if (-not $emccPath) {
        throw "emcc still not available after emsdk install"
    }
    Write-Host "Using emcc: $emccPath"
}

function Invoke-XmlCompile {
    $pageXml = Join-Path $Root "Demo.xml"
    $pageCppName = "DemoPage.cpp"
    $assetDirectory = $null
    if ($App -eq "Desktop") {
        $pageXml = Join-Path (Split-Path -Parent $Root) "RoboDesktop\Desktop.xml"
        $pageCppName = "DesktopPage.cpp"
        $assetDirectory = Join-Path $Root "Assets"
    }
    Write-Step "Compiling $App to C++"
    $vcvars = Find-VcVars64
    if (-not $vcvars) {
        throw "Visual Studio C++ toolchain not found (need vcvars64.bat)"
    }

    $toolDir = Join-Path $Root "Build\XmlCompile"
    Ensure-Directory $toolDir
    $generatedDir = Join-Path $Root "Build\Generated"
    Ensure-Directory $generatedDir

    $toolExe = Join-Path $toolDir "XmlCompile.exe"
    $toolSources = @(
        (Join-Path $Root "XMLCompiler\Source\XmlCompileMain.cpp"),
        (Join-Path $Root "XMLCompiler\Source\XmlParser.cpp"),
        (Join-Path $Root "XMLCompiler\Source\XmlToCpp.cpp"),
        (Join-Path $Root "XMLCompiler\Source\SvgPath.cpp"),
        (Join-Path $Root "XMLCompiler\Source\IconPack.cpp"),
        (Join-Path $Root "Compiler\Source\Color.cpp"),
        (Join-Path $Root "Compiler\Source\Foundation.cpp"),
        (Join-Path $Root "Compiler\Source\RoboFoundation.cpp")
    )
    $rspPath = Join-Path $toolDir "Sources.rsp"
    $rspLines = @(
        "/nologo",
        "/EHsc",
        "/std:c++17",
        "/O2",
        "/I$(Join-Path $Root 'XMLCompiler\Include')",
        "/I$(Join-Path $Root 'Compiler\Include')"
    ) + $toolSources + @(
        "/Fe$($toolExe.Replace('\','/'))",
        "/Fo$($toolDir.Replace('\','/'))/"
    )
    Set-Content -Path $rspPath -Value ($rspLines -join [Environment]::NewLine) -Encoding ASCII

    $batchPath = Join-Path $toolDir "Compile.bat"
    Set-Content -Path $batchPath -Value @"
@echo off
call "$vcvars" >nul
cl @"$rspPath"
exit /b %ERRORLEVEL%
"@ -Encoding ASCII

    & cmd.exe /c $batchPath
    if ($LASTEXITCODE -ne 0) {
        throw "XmlCompile build failed with exit code $LASTEXITCODE"
    }

    $pageCpp = Join-Path $generatedDir $pageCppName
    if ($assetDirectory) {
        & $toolExe $pageXml $pageCpp $assetDirectory
    } else {
        & $toolExe $pageXml $pageCpp
    }
    if ($LASTEXITCODE -ne 0) {
        throw "XmlCompile failed to compile $pageXml"
    }
    Write-Host "Built: $pageCpp"

    if ($App -eq "Desktop") {
        $testAppXml = Join-Path $Root "TestApp\TestApp.xml"
        $testAppCpp = Join-Path $generatedDir "TestAppPage.cpp"
        if (-not (Test-Path $testAppXml)) {
            throw "TestApp\TestApp.xml is missing"
        }
        & $toolExe $testAppXml $testAppCpp $assetDirectory
        if ($LASTEXITCODE -ne 0) {
            throw "XmlCompile failed to compile $testAppXml"
        }
        $testAppText = [IO.File]::ReadAllText($testAppCpp)
        $testAppText = $testAppText.Replace("void BuildPage(", "void BuildTestAppPage(")
        $testAppText = $testAppText.Replace("BuildPage(scene)", "BuildTestAppPage(scene)")
        $testAppText = $testAppText.Replace("int BuildPageStep(", "int BuildTestAppPageStep(")
        [IO.File]::WriteAllText($testAppCpp, $testAppText)
        Write-Host "Built: $testAppCpp"

        # Full compiler libraries stay in Assets; desktop + TestApp packs go to Build.
        & $toolExe --pack (Join-Path $assetDirectory "RoboIcons.pack") (Join-Path $assetDirectory "ROBOICON.PAK")
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to write ROBOICON.PAK"
        }
        & $toolExe --pack (Join-Path $assetDirectory "RoboCountry.pack") (Join-Path $assetDirectory "ROBOCTRY.PAK")
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to write ROBOCTRY.PAK"
        }
        $desktopXml = Join-Path (Split-Path $Root -Parent) "RoboDesktop\Desktop.xml"
        if (-not (Test-Path $desktopXml)) {
            $desktopXml = Join-Path $Root "..\RoboDesktop\Desktop.xml"
        }
        $iconPak = Join-Path $generatedDir "ROBOICON.PAK"
        $countryPak = Join-Path $generatedDir "ROBOCTRY.PAK"
        $catalogPak = Join-Path $generatedDir "ROBOCAT.PAK"
        $panePak = Join-Path $generatedDir "ROBOPANE.PAK"
        & $toolExe --pack-used (Join-Path $assetDirectory "RoboIcons.pack") $iconPak $desktopXml $testAppXml
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to write used ROBOICON.PAK"
        }
        & $toolExe --pack-used (Join-Path $assetDirectory "RoboCountry.pack") $countryPak $desktopXml $testAppXml
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to write used ROBOCTRY.PAK"
        }
        [IO.File]::WriteAllBytes($catalogPak, [byte[]](0x52, 0x43, 0x41, 0x54, 0x31, 0, 0, 0, 0, 0, 0, 0))
        [IO.File]::WriteAllBytes($panePak, [byte[]](0x52, 0x50, 0x41, 0x4E, 0x31, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))
        Write-Host "Built desktop packs in ${generatedDir}: ROBOICON.PAK ROBOCTRY.PAK ROBOCAT.PAK ROBOPANE.PAK"
    }
}

function Invoke-WebBuild {
    $outputDir = $BuildWeb
    $pageCppName = "DemoPage.cpp"
    if ($App -eq "Desktop") {
        $outputDir = Join-Path $Root "Build\Desktop"
        $pageCppName = "DesktopPage.cpp"
    }
    Write-Step "Compiling $App C++ to WebAssembly"
    Ensure-Directory $outputDir

    $sources = (Get-CoreSources) + @(
        (Join-Path $Root "Build\Generated\$pageCppName")
    )
    if ($App -eq "Desktop") {
        $sources += (Join-Path $Root "Build\Generated\TestAppPage.cpp")
    }
    $sources += @(
        (Join-Path $Root "RoboPlatforms\Web\WebPlatform.cpp"),
        (Join-Path $Root "RoboPlatforms\Web\Main.cpp")
    )

    $fontsDir = Join-Path $Root "Assets\Fonts"
    $outputJs = Join-Path $outputDir "Robo.js"
    $preloadArgs = @(
        "--preload-file", "${fontsDir}@/Assets/Fonts"
    )
    if ($App -eq "Desktop") {
        $packed = Join-Path $Root "Assets\Background.rjp"
        if (Test-Path $packed) {
            $preloadArgs += @("--preload-file", "${packed}@/Assets/Background.rjp")
        }
        $logo = Join-Path $Root "Assets\RoboticelaLogo.svg"
        if (Test-Path $logo) {
            $preloadArgs += @("--preload-file", "${logo}@/Assets/RoboticelaLogo.svg")
        }
        $folderIcon = Join-Path (Split-Path $Root -Parent) "Assets\EmptyFolder.svg"
        if (Test-Path $folderIcon) {
            $preloadArgs += @("--preload-file", "${folderIcon}@/Assets/EmptyFolder.svg")
        }
        # Prefer full runtime packs so hosted apps (TestApp) always resolve pack icons.
        $assetsDir = Join-Path $Root "Assets"
        $iconPack = Join-Path $assetsDir "ROBOICON.PAK"
        if (-not (Test-Path $iconPack)) {
            $iconPack = Join-Path $Root "Build\Generated\ROBOICON.PAK"
        }
        $countryPack = Join-Path $assetsDir "ROBOCTRY.PAK"
        if (-not (Test-Path $countryPack)) {
            $countryPack = Join-Path $Root "Build\Generated\ROBOCTRY.PAK"
        }
        $catalogPack = Join-Path $Root "Build\Generated\ROBOCAT.PAK"
        $panePack = Join-Path $Root "Build\Generated\ROBOPANE.PAK"
        if (Test-Path $iconPack) {
            $preloadArgs += @("--preload-file", "${iconPack}@/Assets/ROBOICON.PAK")
        }
        if (Test-Path $countryPack) {
            $preloadArgs += @("--preload-file", "${countryPack}@/Assets/ROBOCTRY.PAK")
        }
        if (Test-Path $catalogPack) {
            $preloadArgs += @("--preload-file", "${catalogPack}@/Assets/ROBOCAT.PAK")
        }
        if (Test-Path $panePack) {
            $preloadArgs += @("--preload-file", "${panePack}@/Assets/ROBOPANE.PAK")
        }
    }

    $emccArgs = @(
        "-O2",
        "-std=c++17",
        "--bind"
    ) + (Get-IncludeArgs "em") + @(
        "-I$(Join-Path $Root 'RoboPlatforms\Web')"
    )
    if ($App -eq "Desktop") {
        $emccArgs += "-DROBO_WEB_DESKTOP=1"
    }
    $emccArgs += $sources + $preloadArgs + @(
        "-sMODULARIZE=1",
        "-sEXPORT_NAME=CreateRoboModule",
        "-sALLOW_MEMORY_GROWTH=1",
        "-sSTACK_SIZE=2097152",
        "-sENVIRONMENT=web",
        "-sFORCE_FILESYSTEM=1",
        "-o", $outputJs
    )

    & em++ @emccArgs
    if ($LASTEXITCODE -ne 0) {
        throw "em++ failed with exit code $LASTEXITCODE"
    }

    $assetUrl = "/Build/Web/"
    $publicDir = Join-Path $Root "Demo"
    if ($App -eq "Desktop") {
        $assetUrl = "/Build/Desktop/"
        $publicDir = Join-Path $Root "Desktop"
    }
    $writer = Join-Path $Root "Tools\WritePageIndex.js"
    & node $writer --out $outputDir --title $App --asset $assetUrl
    if ($LASTEXITCODE -ne 0) {
        throw "Index.html was not written"
    }
    & node $writer --out $publicDir --title $App --asset $assetUrl
    if ($LASTEXITCODE -ne 0) {
        throw "Index.html was not written"
    }

    Write-Host "Built: $outputJs"
    Write-Host "Built: $(Join-Path $outputDir 'Robo.wasm')"
    Write-Host "Built: $(Join-Path $outputDir 'Index.html')"
}

function Find-VcVars64 {
    $known = @(
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    )
    foreach ($path in $known) {
        if (Test-Path $path) {
            return $path
        }
    }

    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $installPath = & $vswhere -latest -products * -property installationPath
        if ($installPath) {
            $vcvars = Join-Path $installPath "VC\Auxiliary\Build\vcvars64.bat"
            if (Test-Path $vcvars) {
                return $vcvars
            }
        }
    }
    return $null
}

Write-Step "RoboDev build ($App to WebAssembly)"
Invoke-XmlCompile
Ensure-Emsdk
Invoke-WebBuild
Write-Step "Build complete"
if ($App -eq "Desktop") {
    Write-Host "Desktop: ..\RunDesktop.ps1"
} else {
    Write-Host "Demo: .\RunDemo.ps1"
}
