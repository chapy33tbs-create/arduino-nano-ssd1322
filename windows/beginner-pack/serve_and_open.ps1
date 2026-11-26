# Local HTTP server + open browser (no install, no unsigned exe flash tool)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

$port = 8765
$url = "http://127.0.0.1:$port/flash.html"

$listener = New-Object System.Net.HttpListener
$listener.Prefixes.Add("http://127.0.0.1:$port/")
try {
  $listener.Start()
} catch {
  Write-Host "Port $port in use. Trying 8766..."
  $port = 8766
  $url = "http://127.0.0.1:$port/flash.html"
  $listener = New-Object System.Net.HttpListener
  $listener.Prefixes.Add("http://127.0.0.1:$port/")
  $listener.Start()
}

Write-Host "Server: $url"
Write-Host "Keep this window open until flash finishes."
Write-Host "Press Ctrl+C to stop."
Write-Host ""

# Open Edge, then Chrome, then default
$opened = $false
foreach ($browser in @(
  "${env:ProgramFiles(x86)}\Microsoft\Edge\Application\msedge.exe",
  "$env:ProgramFiles\Microsoft\Edge\Application\msedge.exe",
  "$env:ProgramFiles\Google\Chrome\Application\chrome.exe",
  "${env:ProgramFiles(x86)}\Google\Chrome\Application\chrome.exe"
)) {
  if (Test-Path $browser) {
    Start-Process -FilePath $browser -ArgumentList $url
    $opened = $true
    break
  }
}
if (-not $opened) {
  Start-Process $url
}

$mime = @{
  '.html' = 'text/html; charset=utf-8'
  '.js'   = 'application/javascript; charset=utf-8'
  '.hex'  = 'text/plain; charset=utf-8'
  '.txt'  = 'text/plain; charset=utf-8'
  '.css'  = 'text/css; charset=utf-8'
}

try {
  while ($listener.IsListening) {
    $ctx = $listener.GetContext()
    $req = $ctx.Request
    $res = $ctx.Response
    $path = [Uri]::UnescapeDataString($req.Url.LocalPath.TrimStart('/'))
    if ([string]::IsNullOrWhiteSpace($path)) { $path = 'flash.html' }
    $full = Join-Path $root ($path -replace '/', '\')

    if (-not (Test-Path -LiteralPath $full -PathType Leaf)) {
      $res.StatusCode = 404
      $buf = [Text.Encoding]::UTF8.GetBytes('404')
      $res.ContentLength64 = $buf.Length
      $res.OutputStream.Write($buf, 0, $buf.Length)
      $res.Close()
      continue
    }

    $ext = [IO.Path]::GetExtension($full).ToLowerInvariant()
    $res.ContentType = $(if ($mime.ContainsKey($ext)) { $mime[$ext] } else { 'application/octet-stream' })
    $bytes = [IO.File]::ReadAllBytes($full)
    $res.ContentLength64 = $bytes.Length
    $res.OutputStream.Write($bytes, 0, $bytes.Length)
    $res.Close()
  }
} finally {
  if ($listener.IsListening) { $listener.Stop() }
  $listener.Close()
}
