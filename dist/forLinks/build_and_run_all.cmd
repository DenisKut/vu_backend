@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

REM === Конфиг-файл ===
set "CONFIG=%~dp0build_and_run_config.ini"
if not exist "%CONFIG%" (
    echo Не найден конфиг build_and_run_config.ini! & pause & exit /b 1
)

REM === Получить значение из ini ===
for /f "tokens=2 delims==" %%A in ('findstr /i "^qt_path[ ]*=" "%CONFIG%"') do set "QT_PATH=%%A"
for /f "tokens=2 delims==" %%A in ('findstr /i "^mingw_path[ ]*=" "%CONFIG%"') do set "MINGW_PATH=%%A"
for /f "tokens=2 delims==" %%A in ('findstr /i "^make_args[ ]*=" "%CONFIG%"') do set "MAKE_ARGS=%%A"
for /f "tokens=2 delims==" %%A in ('findstr /i "^projects[ ]*=" "%CONFIG%"') do set "PROJECTS=%%A"
for /f "tokens=2 delims==" %%A in ('findstr /i "^force_rebuild_backend[ ]*=" "%CONFIG%"') do set "FORCE_REBUILD_BACKEND=%%A"
for /f "tokens=2 delims==" %%A in ('findstr /i "^force_rebuild_rmo[ ]*=" "%CONFIG%"') do set "FORCE_REBUILD_RMO=%%A"
if not defined FORCE_REBUILD_BACKEND set FORCE_REBUILD_BACKEND=0
if not defined FORCE_REBUILD_RMO set FORCE_REBUILD_RMO=0

REM === Проверка путей к Qt/MinGW ===
if "%QT_PATH%"=="" (
    echo Введите путь к папке, где лежит qmake.exe (например, D:\Progs\Qt\5.15.2\mingw81_64\bin):
    set /p QT_PATH="QT bin: "
    set "QT_PATH=%QT_PATH:\"=%"
    if not exist "%QT_PATH%\qmake.exe" echo Путь некорректен! & pause & exit /b 1
)
if "%MINGW_PATH%"=="" (
    echo Введите путь к папке, где лежит mingw32-make.exe (например, D:\Progs\Qt\Tools\mingw810_64\bin):
    set /p MINGW_PATH="MinGW bin: "
    set "MINGW_PATH=%MINGW_PATH:\"=%"
    if not exist "%MINGW_PATH%\mingw32-make.exe" if not exist "%MINGW_PATH%\make.exe" echo Путь некорректен! & pause & exit /b 1
)
set "PATH=%QT_PATH%;%MINGW_PATH%;%PATH%"

REM === Сборка и запуск проектов ===
set ERROR_FOUND=0
for %%P in (%PROJECTS%) do (
    call :build_and_run_project %%P
    if errorlevel 1 set ERROR_FOUND=1
)
if !ERROR_FOUND! == 1 (
    echo Были ошибки при сборке или запуске проектов.
)
pause
goto :eof

:find_exe_path
REM %1 = project name, %2 = config
set "EXE_PATH="
set "P=%1"
set "CFG=%2"
set "CANDIDATES="
if /i "!P!"=="vu_backend" set "CANDIDATES=bin\!CFG!\vu_backend.exe bin\release\vu_backend.exe bin\debug\vu_backend.exe"
if /i "!P!"=="vu_rmo" set "CANDIDATES=build\!CFG!\vu_rmo.exe build\release\vu_rmo.exe build\debug\vu_rmo.exe"
for %%C in (!CANDIDATES!) do if exist "%%C" set "EXE_PATH=%%C" & goto :found_exe
:found_exe
exit /b

:build_and_run_project
set "P=%1"
REM Получить параметры проекта
for /f "tokens=2 delims==" %%A in ('findstr /i "^path[ ]*=" "%CONFIG%" ^| findstr /i "\[!P!\]"') do set "PATH_!P!=%%A"
for /f "tokens=2 delims==" %%A in ('findstr /i "^pro[ ]*=" "%CONFIG%" ^| findstr /i "\[!P!\]"') do set "PRO_!P!=%%A"
for /f "tokens=2 delims==" %%A in ('findstr /i "^exe[ ]*=" "%CONFIG%" ^| findstr /i "\[!P!\]"') do set "EXE_!P!=%%A"
for /f "tokens=2 delims==" %%A in ('findstr /i "^qmake_args[ ]*=" "%CONFIG%" ^| findstr /i "\[!P!\]"') do set "QMAKE_ARGS_!P!=%%A"
for /f "tokens=2 delims==" %%A in ('findstr /i "^make_args[ ]*=" "%CONFIG%" ^| findstr /i "\[!P!\]"') do set "MAKE_ARGS_!P!=%%A"
for /f "tokens=2 delims==" %%A in ('findstr /i "^qmake_config[ ]*=" "%CONFIG%" ^| findstr /i "\[!P!\]"') do set "QMAKE_CONFIG_!P!=%%A"
REM force_rebuild для проекта
for /f "tokens=2 delims==" %%A in ('findstr /i "^force_rebuild_!P![ ]*=" "%CONFIG%"') do set "FORCE_REBUILD=%%A"
if not defined FORCE_REBUILD set FORCE_REBUILD=0
cd /d "%~dp0!PATH_!P!!"
call :find_exe_path !P! !QMAKE_CONFIG_!P!
REM Если не нужно пересобирать и exe найден — просто запуск
if "!FORCE_REBUILD!"=="0" if defined EXE_PATH goto :run_exe
REM Очистка перед сборкой
if /i "!P!"=="vu_backend" if exist Makefile (make cleanall >nul 2>&1)
if /i "!P!"=="vu_rmo" (
    if exist build rmdir /s /q build
    mkdir build >nul 2>&1
    cd build
    qmake ..\!PRO_!P!! !QMAKE_ARGS_!P!! CONFIG+=!QMAKE_CONFIG_!P!!
    make !MAKE_ARGS_!P!!
    cd ..
)
if /i "!P!"=="vu_backend" (
    qmake !QMAKE_ARGS_!P!! CONFIG+=!QMAKE_CONFIG_!P!! !PRO_!P!!
    make !MAKE_ARGS_!P!!
)
REM После сборки ищем exe снова
call :find_exe_path !P! !QMAKE_CONFIG_!P!
:run_exe
if defined EXE_PATH (
    start "!P!" "!EXE_PATH!"
) else (
    echo Не найден исполняемый файл для !P! (искал:)
    if /i "!P!"=="vu_backend" (
        echo   bin\!QMAKE_CONFIG_!P!\vu_backend.exe
        echo   bin\release\vu_backend.exe
        echo   bin\debug\vu_backend.exe
    )
    if /i "!P!"=="vu_rmo" (
        echo   build\!QMAKE_CONFIG_!P!\vu_rmo.exe
        echo   build\release\vu_rmo.exe
        echo   build\debug\vu_rmo.exe
    )
)
goto :eof
