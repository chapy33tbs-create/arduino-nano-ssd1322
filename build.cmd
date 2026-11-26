@echo off
REM ============================================================
REM  Arduino Nano OLED Gauge - build (both bootloader variants)
REM  Usage: build.cmd            -> build all environments
REM         build.cmd -e nanoatmega328
REM         build.cmd -t clean
REM ============================================================
setlocal
set "PIO=%USERPROFILE%\pio-venv\Scripts\platformio.exe"
if not exist "%PIO%" (
  echo [ERROR] PlatformIO not found: %PIO%
  echo         See docs\build-windows.md
  exit /b 1
)
cd /d "%~dp0"
"%PIO%" run %*
set RC=%ERRORLEVEL%
endlocal & exit /b %RC%
