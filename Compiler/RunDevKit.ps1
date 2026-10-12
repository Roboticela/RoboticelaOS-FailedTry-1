$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$WebJs = Join-Path $Root "Build\DevKit\Robo.js"
$Port = 4173
$Url = "http://127.0.0.1:$Port/DevKit/"

& (Join-Path $Root "Build.ps1") -App DevKit
if (-not (Test-Path $WebJs)) {
    throw "DevKit web build was not produced."
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
    $serverPath = Join-Path $Root "Tools\RoboPreviewServer.js"
    Start-Process -FilePath "node" -ArgumentList @($serverPath, "$Port", "/devkit/") -WorkingDirectory $Root -WindowStyle Hidden
    Start-Sleep -Seconds 1
}

Start-Process $Url
Write-Host "DevKit: $Url"
