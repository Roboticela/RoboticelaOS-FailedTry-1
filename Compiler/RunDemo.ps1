$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$WebJs = Join-Path $Root "Build\Web\Robo.js"
$Port = 4173
$Url = "http://127.0.0.1:$Port/Demo/"

& (Join-Path $Root "Build.ps1")
if (-not (Test-Path $WebJs)) {
    throw "Web build was not produced."
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
    Write-Host "Starting demo server on $Url"
    $serverPath = Join-Path $Root "Tools\RoboPreviewServer.js"
    Start-Process -FilePath "node" -ArgumentList @($serverPath, "$Port", "/Demo/") -WorkingDirectory $Root -WindowStyle Hidden
    Start-Sleep -Seconds 1
}

Start-Process $Url
Write-Host "Demo: $Url"
