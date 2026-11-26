@echo off
chcp 65001 >nul
cd /d "%~dp0"
title Oil/Water Gauge — XLoader

echo ========================================
echo   Opening XLoader (GUI flasher)
echo ========================================
echo.
echo If Windows blocks XLoader (Smart App Control),
echo use Arduino IDE instead:
echo   OilWaterGauge\OilWaterGauge.ino
echo See 00_ReadMe_JP.txt section A.
echo.
echo XLoader settings:
echo   Hex file  ... firmware.hex
echo   Device    ... Nano Old Bootloader (recommended)
echo   COM port  ... COMx
echo.
echo Press any key to open XLoader...
pause >nul

if exist "%~dp00_Unblock_files.bat" (
  powershell -NoProfile -ExecutionPolicy Bypass -Command "Get-ChildItem -LiteralPath '%~dp0XLoader' -Recurse -File | Unblock-File -ErrorAction SilentlyContinue" >nul 2>&1
)

start "" "%~dp0XLoader\XLoader.exe"
if errorlevel 1 (
  echo.
  echo Failed to start. Use Arduino IDE path in 00_ReadMe_JP.txt
  pause
  exit /b 1
)

echo.
echo If nothing opened, Smart App Control may be blocking.
echo Use OilWaterGauge\OilWaterGauge.ino with Arduino IDE 2.
echo.
pause
