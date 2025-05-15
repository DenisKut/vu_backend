@echo off
chcp 1251 >nul
setlocal enabledelayedexpansion

if "%~1"=="" (
    set "TAR_DIR=%~dp0"
    echo ========================================================
    echo   ЗАГРУЗКА DOCKER-ОБРАЗОВ ИЗ ТЕКУЩЕЙ ДИРЕКТОРИИ
    echo ========================================================
) else (
    set "TAR_DIR=%~1"
    echo ========================================================
    echo   ЗАГРУЗКА DOCKER-ОБРАЗОВ ИЗ: %TAR_DIR%
    echo ========================================================
)

if not exist "%TAR_DIR%\" (
    echo ? ОШИБКА: Директория не существует: %TAR_DIR%
    pause
    exit /b 1
)

cd /d "%TAR_DIR%"
set "MD5_DIR=md5sums"

set "COUNT=0"
set "SUCCESS=0"
set "FAILED=0"

echo.
echo Обрабатываемая директория: %TAR_DIR%
echo MD5-файлы ищутся в: %TAR_DIR%\%MD5_DIR%
echo ========================================================

for %%G in (*.tar.gz) do (
    set /a COUNT+=1
    set "BASE=%%~nG"
    set "BASE=!BASE:.tar=!"

    echo [Файл !COUNT!] %%G

    set "MD5_GZ_FILE=%MD5_DIR%\!BASE!.tar.gz.md5"
    set "MD5_TAR_FILE=%MD5_DIR%\!BASE!.tar.md5"

    :: 1. Проверка MD5 .tar.gz
    if exist "!MD5_GZ_FILE!" (
        for /f %%H in ('certutil -hashfile "!BASE!.tar.gz" MD5 ^| find /i /v "MD5 hash" ^| find /i /v "CertUtil"') do (
            set "HASH_GZ=%%H"
        )
        for /f "usebackq tokens=1" %%J in ("!MD5_GZ_FILE!") do (
            set "EXPECTED_GZ=%%J"
        )
        if /i "!HASH_GZ!" NEQ "!EXPECTED_GZ!" (
            echo НЕСОВПАДЕНИЕ MD5 .tar.gz: !BASE!.tar.gz
            set /a FAILED+=1
            echo --------------------------------------------------------
            goto :continue
        ) else (
            echo MD5 .tar.gz: OK
        )
    ) else (
        echo Отсутствует: !MD5_GZ_FILE!
        goto :continue
    )

    :: 2. Распаковка .tar.gz
    powershell -Command "tar -xzf '!BASE!.tar.gz'" >nul 2>&1
    if not exist "!BASE!.tar" (
        echo Не удалось распаковать: !BASE!.tar.gz
        set /a FAILED+=1
        echo --------------------------------------------------------
        goto :continue
    )

    :: 3. Проверка MD5 .tar
    if exist "!MD5_TAR_FILE!" (
        for /f %%H in ('certutil -hashfile "!BASE!.tar" MD5 ^| find /i /v "MD5 hash" ^| find /i /v "CertUtil"') do (
            set "HASH_TAR=%%H"
        )
        for /f "usebackq tokens=1" %%J in ("!MD5_TAR_FILE!") do (
            set "EXPECTED_TAR=%%J"
        )
        if /i "!HASH_TAR!" NEQ "!EXPECTED_TAR!" (
            echo НЕСОВПАДЕНИЕ MD5 .tar: !BASE!.tar
            set /a FAILED+=1
            echo --------------------------------------------------------
            goto :continue
        ) else (
            echo MD5 .tar: OK
        )
    ) else (
        echo Отсутствует: !MD5_TAR_FILE!
        goto :continue
    )

    :: 4. docker load
    for /f "delims=" %%D in ('docker load -i "!BASE!.tar" 2^>^&1') do (
        set "output=%%D"
        echo !output!
        echo !output! | findstr /i "loaded" >nul
        if !errorlevel! equ 0 (
            set /a SUCCESS+=1
            echo УСПЕШНО: !BASE!.tar
        ) else (
            echo !output! | findstr /i "error" >nul
            if !errorlevel! equ 0 (
                set /a FAILED+=1
                echo ОШИБКА загрузки: !BASE!.tar
            )
        )
    )

    :: 5. Удаление .tar
    del /f /q "!BASE!.tar" && echo Удалён: !BASE!.tar

    :continue
    echo --------------------------------------------------------
)

if %COUNT%==0 (
    echo ? НЕ НАЙДЕНО .tar.gz файлов в директории: %TAR_DIR%
    pause
    exit /b 1
)

echo.
echo ================ ИТОГ ================
echo Всего архивов: %COUNT%
echo Успешно:       %SUCCESS%
echo Ошибки:        %FAILED%
echo =====================================
pause
