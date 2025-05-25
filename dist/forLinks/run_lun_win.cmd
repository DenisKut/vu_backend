@echo off
REM Универсальная обёртка для запуска PowerShell-скрипта сборки и запуска проектов
REM Гарантирует, что окно не закроется мгновенно, даже если есть ошибки

powershell.exe -NoExit -ExecutionPolicy Bypass -File "%~dp0build_and_run_all.ps1"
