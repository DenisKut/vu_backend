# build_and_run_all.ps1
# Универсальный PowerShell-скрипт для сборки и запуска vu_backend и vu_rmo на Windows
# Аналог build_and_run_all.sh, но для Windows

$ErrorActionPreference = 'Stop'

function Get-IniValue {
    param(
        [string]$Section,
        [string]$Key,
        [string]$File
    )
    $lines = Get-Content $File
    $inSection = $false
    foreach ($line in $lines) {
        $trim = $line.Trim()
        if ($trim -match "^\[$Section\]$") { $inSection = $true; continue }
        if ($inSection -and $trim -match "^\[.*\]$") { break }
        if ($inSection -and $trim -match "^$Key\s*=\s*(.*)") {
            return $matches[1].Trim()
        }
    }
    return $null
}

function Set-IniValue {
    param(
        [string]$Section,
        [string]$Key,
        [string]$Value,
        [string]$File
    )
    $lines = Get-Content $File
    $out = @()
    $inSection = $false
    $found = $false
    for ($i=0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        if ($line.Trim() -like "[$Section]*") {
            $inSection = $true
            $out += $line
            continue
        }
        if ($inSection -and $line.Trim() -match "^\[.*\]$") {
            if (-not $found) { $out += "$Key = $Value"; $found = $true }
            $inSection = $false
        }
        if ($inSection -and $line.Trim() -match "^$Key\s*=") {
            $out += "$Key = $Value"
            $found = $true
            continue
        }
        $out += $line
    }
    if (-not $found) {
        # Section not found or key not found
        $out += "[$Section]"
        $out += "$Key = $Value"
    }
    Set-Content -Path $File -Value $out -Encoding UTF8
}

function Find-QtBin {
    param([string]$BinName)
    Write-Host "[INFO] Ищу $BinName по стандартным путям..."
    $drives = Get-PSDrive -PSProvider FileSystem | Where-Object { $_.Free -gt 0 } | Select-Object -ExpandProperty Root
    $searchDirs = @(
        "$env:USERPROFILE\Qt*\*\bin",
        "C:\\Qt*\\*\\bin",
        "C:\\Program Files\\Qt*\\*\\bin",
        "C:\\tools\\Qt*\\*\\bin",
        "C:\\Qt*\\bin",
        "C:\\tools\\bin",
        "C:\\Program Files\\bin"
    )
    foreach ($drive in $drives) {
        $searchDirs += @(
            "$drive\\Qt*\\*\\bin",
            "$drive\\tools\\Qt*\\*\\bin",
            "$drive\\tools\\bin",
            "$drive\\*\\bin"
        )
    }
    $found = $null
    foreach ($pattern in $searchDirs) {
        $bins = Get-ChildItem -Path $pattern -Filter $BinName -Recurse -ErrorAction SilentlyContinue | Sort-Object FullName
        foreach ($bin in $bins) {
            if ($bin.FullName -match "Qt5") { return $bin.FullName }
        }
    }
    if ($found) { return $found }
    # Fallback: PATH
    $cmd = Get-Command $BinName -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    # Если не найдено — спросить у пользователя, делать ли глубокий поиск
    Write-Host "[WARN] $BinName не найден в стандартных папках и PATH. Выполнить глубокий поиск по всем дискам? (может занять несколько минут) [Y/N]" -ForegroundColor Yellow
    $answer = Read-Host
    if ($answer -match '^[YyАаДд]') {
        Write-Host "[INFO] Глубокий поиск $BinName по всем дискам... Это может занять несколько минут..."
        foreach ($drive in $drives) {
            try {
                $bins = Get-ChildItem -Path $drive -Filter $BinName -Recurse -ErrorAction SilentlyContinue -Depth 6 | Sort-Object FullName
                foreach ($bin in $bins) {
                    if ($bin.FullName -match "Qt5") { return $bin.FullName }
                    if (-not $found) { $found = $bin.FullName }
                }
            } catch {}
        }
        if ($found) { return $found }
    }
    return $null
}

function Find-MakeBin {
    $names = @('mingw32-make.exe', 'make.exe')
    foreach ($name in $names) {
        $found = Find-QtBin $name
        if ($found) { return $found }
    }
    return $null
}

# Определяем корень
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$Root = $ScriptDir
$Ini = Join-Path $Root 'build_and_run_config.ini'

# 1. Поиск qmake
$qmake = Get-IniValue env 'qt_path' $Ini
if (-not $qmake -or -not (Test-Path $qmake)) {
    $qmake = Find-QtBin 'qmake.exe'
    if ($qmake) { Set-IniValue env 'qt_path' $qmake $Ini }
}
if (-not $qmake -or -not (Test-Path $qmake)) {
    Write-Host 'qmake.exe не найден! Укажите путь вручную:'
    $qmake = Read-Host 'Введите полный путь к qmake.exe'
    if ($qmake) { Set-IniValue env 'qt_path' $qmake $Ini }
}
if (-not (Test-Path $qmake)) { Write-Error 'qmake.exe не найден!'; exit 1 }

# 2. Поиск make
$make = Get-IniValue env 'mingw_path' $Ini
if (-not $make -or -not (Test-Path $make)) {
    $make = Find-MakeBin
    if ($make) { Set-IniValue env 'mingw_path' $make $Ini }
}
if (-not $make -or -not (Test-Path $make)) {
    Write-Host 'mingw32-make.exe/make.exe не найден! Укажите путь вручную:'
    $make = Read-Host 'Введите полный путь к mingw32-make.exe или make.exe'
    if ($make) { Set-IniValue env 'mingw_path' $make $Ini }
}
if (-not (Test-Path $make)) { Write-Error 'mingw32-make.exe/make.exe не найден!'; exit 1 }

# 3. make_args
$makeArgs = Get-IniValue env 'make_args' $Ini
if (-not $makeArgs) { $makeArgs = '-j4' }

# 4. force_rebuild
$forceBackend = Get-IniValue env 'force_rebuild_backend' $Ini
$forceRmo = Get-IniValue env 'force_rebuild_rmo' $Ini
if (-not $forceBackend) { $forceBackend = '0' }
if (-not $forceRmo) { $forceRmo = '0' }

# 5. vu_backend
$backendSection = 'vu_backend'
$backendPath = Get-IniValue $backendSection 'path' $Ini
$backendPro = Get-IniValue $backendSection 'pro' $Ini
$backendQmakeArgs = Get-IniValue $backendSection 'qmake_args' $Ini
$backendQmakeConfig = Get-IniValue $backendSection 'qmake_config' $Ini
$backendExeTemplate = Get-IniValue $backendSection 'exe' $Ini
$backendConfigName = ($backendQmakeConfig -split '\s+')[0]
$backendExe = $backendExeTemplate -replace '\$\{qmake_config\}', $backendConfigName
$backendDir = Join-Path $Root $backendPath
$backendProFile = Join-Path $backendDir $backendPro
$backendExePath = Join-Path $Root $backendExe
Write-Host "[DEBUG] exe шаблон из .ini: $backendExeTemplate"
Write-Host "[DEBUG] qmake_config: $backendQmakeConfig, используется: $backendConfigName"
Write-Host "[DEBUG] Ожидаемый путь к vu_backend.exe: $backendExePath"

# 6. vu_rmo
$rmoSection = 'vu_rmo'
$rmoPath = Get-IniValue $rmoSection 'path' $Ini
$rmoPro = Get-IniValue $rmoSection 'pro' $Ini
$rmoQmakeArgs = Get-IniValue $rmoSection 'qmake_args' $Ini
$rmoQmakeConfig = Get-IniValue $rmoSection 'qmake_config' $Ini
$rmoExeTemplate = Get-IniValue $rmoSection 'exe' $Ini
$rmoConfigName = ($rmoQmakeConfig -split '\s+')[0]
$rmoExe = $rmoExeTemplate -replace '\$\{qmake_config\}', $rmoConfigName
$rmoDir = Join-Path $Root $rmoPath
$rmoProFile = Join-Path $rmoDir $rmoPro
$rmoExePath = Join-Path $Root $rmoExe
Write-Host "[DEBUG] exe шаблон из .ini: $rmoExeTemplate"
Write-Host "[DEBUG] qmake_config: $rmoQmakeConfig, используется: $rmoConfigName"
Write-Host "[DEBUG] Ожидаемый путь к vu_rmo.exe: $rmoExePath"

# 7. Сборка vu_backend
Write-Host "=== Сборка vu_backend ==="
Push-Location $backendDir
if ($forceBackend -eq '1' -and (Test-Path 'Makefile')) {
    Write-Host "[INFO] Очистка vu_backend..."
    & $make cleanall
}
if ($forceBackend -eq '1' -or -not (Test-Path $backendExePath)) {
    Write-Host "[INFO] qmake vu_backend..."
    & $qmake $backendQmakeArgs "CONFIG+=$backendQmakeConfig" $backendProFile
    Write-Host "[INFO] make vu_backend..."
    & $make $makeArgs
}
Pop-Location

# 8. Сборка vu_rmo
Write-Host "=== Сборка vu_rmo ==="
Push-Location $rmoDir
if ($forceRmo -eq '1' -and (Test-Path 'build')) {
    Write-Host "[INFO] Очистка vu_rmo..."
    Remove-Item -Recurse -Force 'build'
}
# Удаляем build/Makefile, если он остался
if (Test-Path 'build/Makefile') {
    Write-Host "[DEBUG] Удаляю build/Makefile перед сборкой..."
    Remove-Item -Force 'build/Makefile'
}
if ($forceRmo -eq '1' -or -not (Test-Path $rmoExePath)) {
    if (-not (Test-Path 'build')) { New-Item -ItemType Directory 'build' | Out-Null }
    Push-Location 'build'
    # Формируем команду qmake без лишних пробелов и без -o build/Makefile
    $qmakeCmd = @($qmake)
    if ($rmoQmakeArgs -and $rmoQmakeArgs.Trim()) { $qmakeCmd += $rmoQmakeArgs }
    $qmakeCmd += "CONFIG+=$rmoQmakeConfig"
    $qmakeCmd += "..\$rmoPro"
    Write-Host "[DEBUG] qmake vu_rmo команда: $($qmakeCmd -join ' ')"
    Write-Host "[DEBUG] Тип qmakeCmd: $($qmakeCmd.GetType().FullName)"
    & $qmake @($qmakeCmd[1..($qmakeCmd.Count-1)])
    Write-Host "[INFO] make vu_rmo..."
    & $make $makeArgs
    Pop-Location
}
Pop-Location

# 9. Запуск vu_backend
Write-Host "=== Запуск vu_backend ==="
if (Test-Path $backendExePath -PathType Leaf) {
    Write-Host "[INFO] Запуск vu_backend в новом консольном окне..."
    $cmdLine = "/k `"$backendExePath`""
    Start-Process cmd.exe -ArgumentList $cmdLine -WorkingDirectory (Split-Path $backendExePath)
} else {
    Write-Warning "Не найден исполняемый файл $backendExePath"
}

# 10. Запуск vu_rmo
Write-Host "=== Запуск vu_rmo ==="
if (Test-Path $rmoExePath -PathType Leaf) {
    Write-Host "[INFO] Запуск vu_rmo (GUI)..."
    Start-Process -FilePath $rmoExePath -WorkingDirectory (Split-Path $rmoExePath)
} else {
    Write-Warning "Не найден исполняемый файл $rmoExePath"
}

# 11. Копирование .env, settings.ini, database/init_db.sql для vu_backend
$backendDestDir = Split-Path $backendExePath -Parent
$backendFilesToCopy = @(
    Join-Path $backendDir '.env',
    Join-Path $backendDir 'settings.ini',
    Join-Path $backendDir 'database/init_db.sql'
)
foreach ($srcFile in $backendFilesToCopy) {
    if (Test-Path $srcFile) {
        try {
            Copy-Item $srcFile -Destination $backendDestDir -Force
            Write-Host "[INFO] Скопирован $srcFile в $backendDestDir"
        } catch {
            Write-Warning ("Не удалось скопировать $srcFile : " + $error[0].Exception.Message)
        }
    } else {
        Write-Warning "Файл $srcFile не найден для копирования в $backendDestDir"
    }
}

Write-Host "Готово!"
