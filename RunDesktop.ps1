$ErrorActionPreference = "Stop"

$Root = $PSScriptRoot
$Compiler = Join-Path $Root "Compiler"
$WebJs = Join-Path $Compiler "Build\Desktop\Robo.js"
$Port = 4173
$Url = "http://127.0.0.1:$Port/Desktop/"

& (Join-Path $Compiler "Build.ps1") -App Desktop
if (-not (Test-Path $WebJs)) {
    throw "Desktop web build was not produced."
}

$listenerRunning = $false
try {
    $existing = Get-NetTCPConnection -LocalPort $Port -State Listen -ErrorAction SilentlyContinue
    if ($existing) {
        $listenerRunning = $true
    }
} catch {
    $listenerRunning = $false
}

if (-not $listenerRunning) {
    Write-Host "Starting server on $Url"
    $serverPath = Join-Path $Compiler "Tools\RoboPreviewServer.js"
    Start-Process -FilePath "node" -ArgumentList @($serverPath, "$Port", "/Desktop/") -WorkingDirectory $Compiler -WindowStyle Hidden
    Start-Sleep -Seconds 1
}

Start-Process $Url
Write-Host "Desktop: $Url"
