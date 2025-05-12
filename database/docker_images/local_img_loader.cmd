@echo off
chcp 1251 >nul
setlocal enabledelayedexpansion

:: Если путь не указан - используем текущую директорию
if "%~1"=="" (
    set "TAR_DIR=%~dp0"
    echo ========================================================
    echo    ЗАГРУЗКА DOCKER-ОБРАЗОВ ИЗ ТЕКУЩЕЙ ДИРЕКТОРИИ
    echo ========================================================
) else (
    set "TAR_DIR=%~1"
    echo ========================================================
    echo    ЗАГРУЗКА DOCKER-ОБРАЗОВ ИЗ УКАЗАННОЙ ДИРЕКТОРИИ
    echo ========================================================
)

:: Проверка существования директории
if not exist "%TAR_DIR%\" (
    echo ОШИБКА: Директория не существует!
    echo Проверьте путь: %TAR_DIR%
    pause
    exit /b 1
)

set "COUNT=0"
set "SUCCESS=0"
set "FAILED=0"

echo.
echo Обрабатываемая директория: %TAR_DIR%
echo ========================================================

for /f "delims=" %%F in ('dir /b /a-d "%TAR_DIR%\*.tar" 2^>nul') do (
    set /a "COUNT+=1"
    echo [Файл !COUNT!] %%F
    
    for /f "delims=" %%I in ('docker load -i "%TAR_DIR%\%%F" 2^>^&1') do (
        set "output=%%I"
        echo !output!
        
        :: Проверка успешной загрузки
        echo !output! | findstr /i "loaded" >nul
        if !errorlevel! equ 0 (
            set /a "SUCCESS+=1"
            echo [Статус] УСПЕШНО
        ) else (
            echo !output! | findstr /i "error" >nul
            if !errorlevel! equ 0 (
                set /a "FAILED+=1"
                echo [Статус] ОШИБКА
            )
        )
    )
    echo --------------------------------------------------------
)

if %COUNT% equ 0 (
    echo НЕ НАЙДЕНО .tar-файлов в указанной директории
    pause
    exit /b 1
)

echo.
echo ================ ИТОГОВАЯ СТАТИСТИКА ================
echo Всего обработано файлов: %COUNT%
echo Успешно загружено: %SUCCESS%
echo Ошибок загрузки: %FAILED%
echo =====================================================
pause