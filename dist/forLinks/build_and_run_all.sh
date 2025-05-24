#!/bin/bash
# Универсальный скрипт для сборки и запуска vu_backend и vu_rmo
set -e

# Определяем корень (где лежит этот скрипт)
ROOT="$(cd "$(dirname "$0")" && pwd)"

# Функция для чтения параметра из ini (Bash)
get_config_param() {
    local section="$1" key="$2" file="$3"
    if [ "$section" = "env" ]; then
        awk -F '=' -v key="$key" '
            $1 ~ key {gsub(/^[ \t]+|[ \t]+$/, "", $2); print $2; exit}
        ' "$file"
    else
        awk -F '=' -v section="[$section]" -v key="$key" '
            $0 ~ section {found=1; next}
            found && $1 ~ key {gsub(/^[ \t]+|[ \t]+$/, "", $2); print $2; exit}
        ' "$file"
    fi
}

# Универсальная функция получения force_rebuild_<project>
get_force_rebuild() {
    local project="$1"
    get_config_param env force_rebuild_${project} "$ROOT/build_and_run_config.ini"
}

# === Поиск qmake и make, если не заданы ===
# Функция поиска бинарника по маске в популярных и редких местах
find_bin() {
    local binname="$1"
    # Популярные пути
    for d in /usr/bin /usr/local/bin /opt/qt*/bin /opt/Qt*/bin /opt/tools/Qt*/bin $HOME/Qt*/bin $HOME/tools/Qt*/bin; do
        [ -x "$d/$binname" ] && echo "$d/$binname" && return 0
    done
    # Поиск по всем смонтированным точкам (например, /mnt/d/Qt*/bin)
    for mnt in /mnt/* /media/*; do
        for d in "$mnt"/Qt*/bin "$mnt"/tools/Qt*/bin; do
            [ -x "$d/$binname" ] && echo "$d/$binname" && return 0
        done
    done
    # find по всем файловым системам, но не глубже 4 уровней (быстро)
    find / /mnt /media $HOME -maxdepth 4 -type f -name "$binname" 2>/dev/null | head -n1
}

[ -z "$QMAKE_BIN" ] && QMAKE_BIN=$(find_bin qmake)
[ -z "$MAKE_BIN" ] && MAKE_BIN=$(find_bin make)

# Можно явно задать переменные окружения, если нужно:
# export QMAKE_BIN=/custom/path/to/qmake
# export MAKE_BIN=/custom/path/to/make

# Флаги пересборки
FORCE_REBUILD_BACKEND=$(get_config_param env force_rebuild_backend "$ROOT/build_and_run_config.ini")
FORCE_REBUILD_RMO=$(get_config_param env force_rebuild_rmo "$ROOT/build_and_run_config.ini")
[ -z "$FORCE_REBUILD_BACKEND" ] && FORCE_REBUILD_BACKEND=0
[ -z "$FORCE_REBUILD_RMO" ] && FORCE_REBUILD_RMO=0

echo "[DEBUG] force_rebuild_backend = $(get_force_rebuild backend)"
echo "[DEBUG] force_rebuild_rmo = $(get_force_rebuild rmo)"

# Сборка vu_backend
cd "$ROOT/vu_backend"
QMAKE_CONFIG_BACKEND=$(get_config_param vu_backend qmake_config "$ROOT/build_and_run_config.ini")
MAKE_ARGS=$(get_config_param env make_args "$ROOT/build_and_run_config.ini")
# Удаляем все невидимые символы и лишние пробелы
MAKE_ARGS="$(echo "$MAKE_ARGS" | tr -d '\r\n\t' | xargs)"
echo "[DEBUG] MAKE_ARGS после очистки: '$MAKE_ARGS'"
echo "[DEBUG] MAKE_ARGS после очистки (правильно): '$MAKE_ARGS'"
echo "[DEBUG] MAKE_ARGS (hex): $(echo -n "$MAKE_ARGS" | xxd -ps)"
# Если пусто — подставить -j4
[ -z "$MAKE_ARGS" ] && MAKE_ARGS="-j4"
# Исправляем все -j без числа на -j4
MAKE_ARGS="$(echo "$MAKE_ARGS" | sed -E 's/(^|[[:space:]])-j($|[[:space:]])/ -j4 /g')"
if [[ "$MAKE_ARGS" =~ (^|[[:space:]])-j($|[[:space:]]) ]]; then
    MAKE_ARGS="${MAKE_ARGS/-j/-j4}"
fi
echo "[DEBUG] MAKE_ARGS после обработки: '$MAKE_ARGS'"
FORCE_REBUILD=$(get_force_rebuild backend)
[ -z "$FORCE_REBUILD" ] && FORCE_REBUILD=0
if [ "$FORCE_REBUILD" = "1" ]; then
    echo "[DEBUG] Пересборка vu_backend по флагу force_rebuild_backend=1"
    if [ -f Makefile ]; then
        ${MAKE_BIN:-make} cleanall || true
    fi
fi
BIN_BACKEND=""
for p in "${BIN_PATHS_BACKEND[@]}"; do
    if [ -x "$p/vu_backend" ]; then
        BIN_BACKEND="$p/vu_backend"
        break
    fi
    if [ -f "$p/vu_backend" ]; then
        chmod +x "$p/vu_backend"
        BIN_BACKEND="$p/vu_backend"
        break
    fi
    if [ -f "$p/vu_backend.exe" ]; then
        BIN_BACKEND="$p/vu_backend.exe"
        break
    fi
    # Для debug/release без qmake_config
    if [ -x "$ROOT/vu_backend/bin/debug/vu_backend" ]; then
        BIN_BACKEND="$ROOT/vu_backend/bin/debug/vu_backend"
        break
    fi
    if [ -x "$ROOT/vu_backend/bin/release/vu_backend" ]; then
        BIN_BACKEND="$ROOT/vu_backend/bin/release/vu_backend"
        break
    fi
    if [ -f "$ROOT/vu_backend/bin/debug/vu_backend.exe" ]; then
        BIN_BACKEND="$ROOT/vu_backend/bin/debug/vu_backend.exe"
        break
    fi
    if [ -f "$ROOT/vu_backend/bin/release/vu_backend.exe" ]; then
        BIN_BACKEND="$ROOT/vu_backend/bin/release/vu_backend.exe"
        break
    fi
    # ...можно добавить ещё варианты
    done

if [ "$FORCE_REBUILD" = "1" ] || [ ! -x "$BIN_BACKEND" ]; then
    if [ -f vu_backend.pro ]; then
        if ${MAKE_BIN:-make} -n cleanall 2>/dev/null | grep -q cleanall; then
            ${MAKE_BIN:-make} cleanall || true
        fi
        QMAKE_ARGS=$(get_config_param vu_backend qmake_args "$ROOT/build_and_run_config.ini")
        [ -n "$QMAKE_CONFIG_BACKEND" ] && QMAKE_CONFIG_ARG="CONFIG+=$QMAKE_CONFIG_BACKEND" || QMAKE_CONFIG_ARG=""
        $QMAKE_BIN $QMAKE_ARGS $QMAKE_CONFIG_ARG vu_backend.pro
        $MAKE_BIN $MAKE_ARGS
        # После сборки ищем бинарник снова
        BIN_BACKEND=""
        BACKEND_PATHS=(
            "$ROOT/vu_backend/bin/$QMAKE_CONFIG_BACKEND/vu_backend"
            "$ROOT/vu_backend/bin/$QMAKE_CONFIG_BACKEND/vu_backend.exe"
            "$ROOT/vu_backend/bin/release/vu_backend"
            "$ROOT/vu_backend/bin/release/vu_backend.exe"
            "$ROOT/vu_backend/bin/debug/vu_backend"
            "$ROOT/vu_backend/bin/debug/vu_backend.exe"
            "$ROOT/vu_backend/vu_backend"
            "$ROOT/vu_backend/vu_backend.exe"
        )
        echo "[DEBUG] Проверяем пути для vu_backend:"
        for p in "${BACKEND_PATHS[@]}"; do
            echo "  $p"
            if [ -x "$p" ]; then BIN_BACKEND="$p"; break; fi
            if [ -f "$p" ]; then chmod +x "$p"; BIN_BACKEND="$p"; break; fi
        done
    else
        echo "vu_backend.pro не найден в vu_backend!"
        exit 1
    fi
fi

# Сборка vu_rmo
cd "$ROOT/vu_rmo"
QMAKE_CONFIG_RMO=$(get_config_param vu_rmo qmake_config "$ROOT/build_and_run_config.ini")
MAKE_ARGS=$(get_config_param env make_args "$ROOT/build_and_run_config.ini")
MAKE_ARGS="$(echo "$MAKE_ARGS" | tr -d '\r\n\t' | sed 's/  */ /g' | sed 's/^ *//;s/ *$//')"
echo "[DEBUG] MAKE_ARGS (hex): $(echo -n "$MAKE_ARGS" | xxd)"
# Если пусто — подставить -j4
[ -z "$MAKE_ARGS" ] && MAKE_ARGS="-j4"
# Исправляем все -j без числа на -j4
MAKE_ARGS="$(echo "$MAKE_ARGS" | sed -E 's/(^|[[:space:]])-j($|[[:space:]])/ -j4 /g')"
if [[ "$MAKE_ARGS" =~ (^|[[:space:]])-j($|[[:space:]]) ]]; then
    MAKE_ARGS="${MAKE_ARGS/-j/-j4}"
    echo "[DEBUG] Исправлен make_args: $MAKE_ARGS"
fi
FORCE_REBUILD=$(get_force_rebuild rmo)
[ -z "$FORCE_REBUILD" ] && FORCE_REBUILD=0
if [ "$FORCE_REBUILD" = "1" ]; then
    echo "[DEBUG] Пересборка vu_rmo по флагу force_rebuild_rmo=1"
    rm -rf build
fi
BIN_RMO=""
for p in "${BIN_PATHS_RMO[@]}"; do
    if [ -x "$p/vu_rmo" ]; then BIN_RMO="$p/vu_rmo"; break; fi
    if [ -f "$p/vu_rmo" ]; then chmod +x "$p/vu_rmo"; BIN_RMO="$p/vu_rmo"; break; fi
    if [ -f "$p/vu_rmo.exe" ]; then BIN_RMO="$p/vu_rmo.exe"; break; fi
    if [ -x "$ROOT/vu_rmo/build/debug/vu_rmo" ]; then BIN_RMO="$ROOT/vu_rmo/build/debug/vu_rmo"; break; fi
    if [ -x "$ROOT/vu_rmo/build/release/vu_rmo" ]; then BIN_RMO="$ROOT/vu_rmo/build/release/vu_rmo"; break; fi
    if [ -f "$ROOT/vu_rmo/build/debug/vu_rmo.exe" ]; then BIN_RMO="$ROOT/vu_rmo/build/debug/vu_rmo.exe"; break; fi
    if [ -f "$ROOT/vu_rmo/build/release/vu_rmo.exe" ]; then BIN_RMO="$ROOT/vu_rmo/build/release/vu_rmo.exe"; break; fi
    done

if [ "$FORCE_REBUILD" = "1" ] || [ ! -x "$BIN_RMO" ]; then
    if [ -f vu_rmo.pro ]; then
        rm -rf build
        mkdir build
        QMAKE_ARGS=$(get_config_param vu_rmo qmake_args "$ROOT/build_and_run_config.ini")
        [ -n "$QMAKE_CONFIG_RMO" ] && QMAKE_CONFIG_ARG="CONFIG+=$QMAKE_CONFIG_RMO" || QMAKE_CONFIG_ARG=""
        cd build
        $QMAKE_BIN $QMAKE_ARGS $QMAKE_CONFIG_ARG ../vu_rmo.pro
        $MAKE_BIN $MAKE_ARGS
        cd ..
        # После сборки ищем бинарник снова
        BIN_RMO=""
        RMO_PATHS=(
            "$ROOT/vu_rmo/build/$QMAKE_CONFIG_RMO/vu_rmo"
            "$ROOT/vu_rmo/build/$QMAKE_CONFIG_RMO/vu_rmo.exe"
            "$ROOT/vu_rmo/build/release/vu_rmo"
            "$ROOT/vu_rmo/build/release/vu_rmo.exe"
            "$ROOT/vu_rmo/build/debug/vu_rmo"
            "$ROOT/vu_rmo/build/debug/vu_rmo.exe"
            "$ROOT/vu_rmo/vu_rmo"
            "$ROOT/vu_rmo/vu_rmo.exe"
        )
        echo "[DEBUG] Проверяем пути для vu_rmo:"
        for p in "${RMO_PATHS[@]}"; do
            echo "  $p"
            if [ -x "$p" ]; then BIN_RMO="$p"; break; fi
            if [ -f "$p" ]; then chmod +x "$p"; BIN_RMO="$p"; break; fi
        done
    else
        echo "vu_rmo.pro не найден в vu_rmo!"
        exit 1
    fi
fi

# Установить XDG_RUNTIME_DIR для Qt (устраняет предупреждение)
export XDG_RUNTIME_DIR="/tmp/runtime-$USER"
mkdir -p "$XDG_RUNTIME_DIR"
chmod 700 "$XDG_RUNTIME_DIR"

# Запуск vu_backend
if [ -x "$BIN_BACKEND" ]; then
    if [ "$EUID" -ne 0 ]; then
        echo "Запуск vu_backend с правами sudo (может потребоваться пароль)..."
        sudo XDG_RUNTIME_DIR="$XDG_RUNTIME_DIR" "$BIN_BACKEND" &
    else
        "$BIN_BACKEND" &
    fi
else
    echo "vu_backend не найден! Пробовал:"
    for p in "${BIN_PATHS_BACKEND[@]}"; do echo "  $p/vu_backend"; done
fi

# Запуск vu_rmo
if [ -x "$BIN_RMO" ]; then
    # if [ "$EUID" -ne 0 ]; then
    #     echo "Запуск vu_rmo с правами sudo (может потребоваться пароль)..."
    #     sudo XDG_RUNTIME_DIR="$XDG_RUNTIME_DIR" "$BIN_RMO" &
    # else
        "$BIN_RMO" &
    # fi
else
    echo "vu_rmo не найден! Пробовал:"
    for p in "${BIN_PATHS_RMO[@]}"; do echo "  $p/vu_rmo"; done
fi

wait
