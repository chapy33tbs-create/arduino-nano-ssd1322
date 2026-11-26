@echo off
REM ============================================================
REM  Arduino Nano OLED Gauge - upload
REM  Usage: upload.cmd                 -> auto-detect port, old bootloader
REM         upload.cmd COM3            -> specify port
REM         upload.cmd COM3 nanoatmega328new
REM  (board not detected? pass the COM port explicitly)
REM ============================================================
setlocal
set "PIO=%USERPROFILE%\pio-venv\Scripts\platformio.exe"
if not exist "%PIO%" (
  echo [ERROR] PlatformIO not found: %PIO%
  exit /b 1
)
cd /d "%~dp0"

set "PORT=%~1"
set "ENV=%~2"
if "%ENV%"=="" set "ENV=nanoatmega328"

if "%PORT%"=="" (
  "%PIO%" run -e %ENV% -t upload
) else (
  "%PIO%" run -e %ENV% -t upload --upload-port %PORT%
)
set RC=%ERRORLEVEL%
endlocal & exit /b %RC%
