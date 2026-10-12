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
    $serverScript = @"
const http = require('http');
const fs = require('fs');
const path = require('path');
const root = process.cwd();
const port = $Port;
const types = {
  '.html': 'text/html; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.wasm': 'application/wasm',
  '.data': 'application/octet-stream',
  '.ttf': 'font/ttf'
};
const server = http.createServer((req, res) => {
  try {
    let urlPath = decodeURIComponent((req.url || '/').split('?')[0]);
    if (urlPath === '/') urlPath = '/Demo/';
    if (urlPath.endsWith('/')) urlPath += 'Index.html';
    const filePath = path.normalize(path.join(root, urlPath));
    if (!filePath.startsWith(root)) {
      res.writeHead(403); res.end('Forbidden'); return;
    }
    if (!fs.existsSync(filePath) || fs.statSync(filePath).isDirectory()) {
      res.writeHead(404); res.end('Not found'); return;
    }
    const ext = path.extname(filePath).toLowerCase();
    res.writeHead(200, {
      'Content-Type': types[ext] || 'application/octet-stream',
      'Cache-Control': 'no-store'
    });
    fs.createReadStream(filePath).pipe(res);
  } catch (error) {
    res.writeHead(500); res.end(String(error));
  }
});
server.listen(port, '127.0.0.1', () => console.log('Robo demo at http://127.0.0.1:' + port + '/Demo/'));
"@
    $serverPath = Join-Path $Root "Build\DemoServer.js"
    New-Item -ItemType Directory -Force -Path (Join-Path $Root "Build") | Out-Null
    Set-Content -Path $serverPath -Value $serverScript -Encoding UTF8
    Start-Process -FilePath "node" -ArgumentList $serverPath -WorkingDirectory $Root -WindowStyle Hidden
    Start-Sleep -Seconds 1
}

Start-Process $Url
Write-Host "Demo: $Url"
