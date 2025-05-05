@echo off
setlocal enabledelayedexpansion

:: Проверка аргумента
if "%~1"=="" (
    echo Использование: %~nx0 ПУТЬ_К_ДИРЕКТОРИИ_С_TAR
    exit /b 1
)

set "TAR_DIR=%~1"

:: Проверка существования директории
if not exist "%TAR_DIR%" (
    echo Ошибка: директория "%TAR_DIR%" не существует
    exit /b 1
)

set "FOUND=false"

for %%F in ("%TAR_DIR%\*.tar") do (
    if exist "%%~fF" (
        set "FOUND=true"
        echo → %%~fF
        docker load -i "%%~fF" >nul 2>&1
        if !errorlevel! equ 0 (
            echo ✓ Загружен: %%~fF
        ) else (
            echo ✗ Ошибка загрузки: %%~fF
        )
    )
)

if "%FOUND%"=="false" (
    echo Нет .tar файлов в директории
)