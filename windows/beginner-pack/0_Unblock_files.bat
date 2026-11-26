@echo off
chcp 65001 >nul
cd /d "%~dp0"
title Unblock files (Mark of the Web)

echo ========================================
echo   Windows の「ブロック解除」
echo ========================================
echo.
echo ダウンロード zip 由来の警告を外します。
echo Smart App Control 自体はオフになりません。
echo.
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "Get-ChildItem -LiteralPath '%~dp0' -Recurse -File | Unblock-File -ErrorAction SilentlyContinue;" ^
  "Write-Host 'Unblock done.'"
echo.
echo 完了。次は 00_ReadMe_JP.txt の手順へ。
pause
