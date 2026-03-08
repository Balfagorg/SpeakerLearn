@echo off
REM FreqSync local runner — backend + frontend for testing
cd /d "%~dp0"
python run_local.py
if errorlevel 1 pause
