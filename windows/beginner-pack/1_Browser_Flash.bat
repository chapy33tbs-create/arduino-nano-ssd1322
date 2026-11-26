@echo off
chcp 65001 >nul
cd /d "%~dp0"
title Oil/Water Gauge — Browser Flash

echo ========================================
echo   ブラウザで書き込み（インストール不要）
echo ========================================
echo.
echo ・exe を実行しません（Smart App Control 対策）
echo ・Edge または Chrome が必要です
echo ・この黒い画面は書き込みが終わるまで開いたまま
echo.

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0serve_and_open.ps1"
if errorlevel 1 (
  echo.
  echo 起動に失敗しました。Edge/Chrome が入っているか確認してください。
  pause
)
