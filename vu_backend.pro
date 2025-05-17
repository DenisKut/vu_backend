QT += core network sql websockets

TARGET = vu_backend
CONFIG += c++11 cmdline
# -------------------------------
# 0. Пути сборки проекта
# -------------------------------

# Определяем тип сборки
BUILD_TYPE = debug
CONFIG(release, debug|release) {
    BUILD_TYPE = release
}

message(Build type: $$BUILD_TYPE)

BIN_DIR = $$OUT_PWD/bin

DESTDIR = $$BIN_DIR/$$BUILD_TYPE
OBJECTS_DIR = $$PWD/objects/$$BUILD_TYPE
MOC_DIR = $$OBJECTS_DIR
UI_DIR = $$OBJECTS_DIR/ui
RCC_DIR = $$OBJECTS_DIR/rcc

QMAKE_CLEAN += \
    object_script.*.Debug \
    object_script.*.Release \
    .qmake.stash

#-------------------------------
# 1. Платформозависимые настройки
#-------------------------------

# Общие для всех платформ настройки
SOURCES += \
    main.cpp \
    pch.cpp \
    common/CoreManager.cpp \
    common/calculator/calculator.cpp \
    common/logger/Logger.cpp \
    network/NetworkManager.cpp \
    network/handlers/EthHandler.cpp \
    network/handlers/TcpConnectionHandler.cpp \
    network/handlers/TcpHandler.cpp \
    network/handlers/UdpHandler.cpp \
    utils/time_provider/TimeProvider.cpp \
    utils/env_loader/EnvLoader.cpp \
    utils/env_loader/Settings.cpp 

HEADERS += \
    common/CoreManager.h \
    common/calculator/calculator.h \
    common/logger/Logger.h \
    network/NetworkManager.h \
    network/handlers/EthHandler.h \
    network/handlers/TcpConnectionHandler.h \
    network/handlers/TcpHandler.h \
    network/handlers/UdpHandler.h \
    network/messages/EthMessage.h \
    network/messages/Message.h \
    network/messages/RmoMessage.h \
    network/messages/TcpMessage.h \
    utils/AppConfig.h \
    utils/time_provider/TimeProvider.h \
    utils/env_loader/EnvLoader.h \
    utils/env_loader/Settings.h 

#глобальное включение
PRECOMPILED_HEADER = pch.h
CONFIG+=precompile_header
INCLUDEPATH += $$PWD/core/logger

# Windows (NPcap)
win32 {
    INCLUDEPATH += $$PWD/libs/wpdpack/Include

    # Определяем архитектуру
    contains(QMAKE_HOST.arch, x86_64) {
        # 64-битная сборка
        LIBS += -L$$PWD/libs/wpdpack/Lib/x64 \
                -lPacket \
                -lwpcap
    } else {
        # 32-битная сборка
        LIBS += -L$$PWD/libs/wpdpack/Lib \
                -lPacket \
                -lwpcap
    }

    DEFINES += WPCAP HAVE_REMOTE
}

#-------------------------------
# 2. Кастомные цели сборки
#-------------------------------

# Цель для документации
docs.command = doxygen Doxyfile
QMAKE_EXTRA_TARGETS += docs

# Цель для деплоя (пример)
deploy.command = echo "Deploying..." && xcopy /Y /S $$OUT_PWD/*.exe C:/deploy/
QMAKE_EXTRA_TARGETS += deploy

#-------------------------------
# 3. Управление функциями через defines
#-------------------------------
CONFIG += debug_build: DEFINES += DEBUG_OUTPUT=1
CONFIG += verbose: DEFINES += VERBOSE_LOGGING=1

#-------------------------------
# 4. Дополнительные настройки
#-------------------------------
DISTFILES += \
    docs/db_info.md \
    docs/dev_info.md \
    docs/git_man.md \
    docs/doxygen_man.md \
    libs/wpdpack/Lib/Packet.lib \
    libs/wpdpack/Lib/libpacket.a \
    libs/wpdpack/Lib/libwpcap.a \
    libs/wpdpack/Lib/wpcap.lib \
    libs/wpdpack/Lib/x64/Packet.lib \
    libs/wpdpack/Lib/x64/wpcap.lib \
    libs/json/json.hpp \
    readme.md \
    .env

#-------------------------------
# 5. Копирование .env в выходную директорию
#-------------------------------

# Копирование .env, config.ini, database/init_db.sql
win32 {
    QMAKE_POST_LINK += $$quote(cmd /c if not exist $$DESTDIR md $$DESTDIR \
        & copy /Y $$shell_path($$PWD/.env) $$shell_path($$DESTDIR) \
        & copy /Y $$shell_path($$PWD/settings.ini) $$shell_path($$DESTDIR) \
        & copy /Y $$shell_path($$PWD/database/init_db.sql) $$shell_path($$DESTDIR))
} else {
    QMAKE_POST_LINK += $$quote(mkdir -p $$DESTDIR && \
        cp -f $$shell_path($$PWD/.env) $$shell_path($$DESTDIR) && \
        cp -f $$shell_path($$PWD/settings.ini) $$shell_path($$DESTDIR) && \
        cp -f $$shell_path($$PWD/database/init_db.sql) $$shell_path($$DESTDIR))
}

win32 {
    QMAKE_POST_CLEAN += rmdir /S /Q $$shell_path($$BIN_DIR)
    QMAKE_POST_CLEAN += rmdir /S /Q $$shell_path($$OBJECTS_DIR)
}

# -------------------------------
# 6. Полная очистка при distclean
# --------------------------------

QMAKE_DISTCLEAN += \
    $$BIN_DIR \
    $$OBJECTS_DIR \
    $$DESTDIR \
    $$DESTDIR/.env \
    $$DESTDIR/settings.ini \
    $$DESTDIR/init_db.sql \
    release \
    debug \
    objects \
    .qmake.stash \
    Makefile \
    Makefile.Debug \
    Makefile.Release \
    object_script.*.Debug \
    object_script.*.Release

# Для рекурсивного удаления директорий
win32 {
    cleanall.commands = \
        cmd /c "if exist $$shell_path($$BIN_DIR) rmdir /S /Q $$shell_path($$BIN_DIR) & \
        if exist $$shell_path($$OBJECTS_DIR) rmdir /S /Q $$shell_path($$OBJECTS_DIR) & \
        if exist release rmdir /S /Q release & \
        if exist debug rmdir /S /Q debug & \
        if exist objects rmdir /S /Q objects & \
        del /F /Q .qmake.stash Makefile* object_script.*.Debug object_script.*.Release"
} else {
    cleanall.commands = \
        rm -rf $$BIN_DIR $$OBJECTS_DIR release debug objects .qmake.stash Makefile* object_script.*.Debug object_script.*.Release
}

QMAKE_EXTRA_TARGETS += cleanall

# ===================================
# !!!!!! Очистка дубликатов !!!!!!
# ===================================
DISTFILES = $$unique(DISTFILES)
HEADERS = $$unique(HEADERS)

# -------------------------------
# 7. Цель trace. Определяет макрос TRACE
# --------------------------------


# Если CONFIG содержит trace, добавим макрос
contains(CONFIG, trace) {
    message(Trace mode enabled)
    DEFINES += TRACE
}