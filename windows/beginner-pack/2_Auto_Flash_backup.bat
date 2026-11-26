@echo off
chcp 65001 >nul
cd /d "%~dp0"
title Oil/Water Gauge backup flash

echo ========================================
echo   Backup auto-flash (if XLoader fails)
echo ========================================
echo.

if not exist "firmware.hex" (
  echo [ERROR] firmware.hex not found.
  pause
  exit /b 1
)
if not exist "XLoader\avrdude.exe" (
  echo [ERROR] XLoader\avrdude.exe not found.
  pause
  exit /b 1
)

echo Connect Nano via USB.
echo.
echo --- COM ports ---
mode | findstr "COM"
powershell -NoProfile -Command "Get-CimInstance Win32_SerialPort | Select-Object -ExpandProperty DeviceID" 2>nul
echo.

set /p PORT=Enter COM number (e.g. 3) ^> 
if "%PORT%"=="" (
  echo Cancelled.
  pause
  exit /b 1
)
echo %PORT%| findstr /I /R "^COM[0-9]" >nul
if errorlevel 1 set PORT=COM%PORT%

echo.
echo Flashing %PORT% Old Bootloader 57600...
XLoader\avrdude.exe -C XLoader\avrdude.conf -v -patmega328p -carduino -P %PORT% -b57600 -D -Uflash:w:firmware.hex:i
if %errorlevel%==0 goto success

echo.
echo Trying New Bootloader 115200...
XLoader\avrdude.exe -C XLoader\avrdude.conf -v -patmega328p -carduino -P %PORT% -b115200 -D -Uflash:w:firmware.hex:i
if %errorlevel%==0 goto success

echo.
echo [FAIL] Check COM / USB / drivers.
pause
exit /b 1

:success
echo.
echo DONE
pause
exit /b 0
