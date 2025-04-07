QT += core network

TARGET = lun_vu
CONFIG += c++11 cmdline

#-------------------------------
# 1. Платформозависимые настройки
#-------------------------------

# Общие для всех платформ настройки
SOURCES += \
    core/CoreManager.cpp \
    core/logger/Logger.cpp \
    main.cpp \
    network/NetworkManager.cpp \
    network/handlers/EthHandler.cpp \
    network/handlers/TcpHandler.cpp \
    network/handlers/UdpHandler.cpp \
    utils/time_provider/TimeProvider.cpp \
    utils/env_loader/EnvLoader.cpp

HEADERS += \
    core/CoreManager.h \
    core/logger/Logger.h \
    network/NetworkManager.h \
    network/handlers/EthHandler.h \
    network/handlers/TcpHandler.h \
    network/handlers/UdpHandler.h \
    network/messages/EthMessage.h \
    network/messages/Message.h \
    network/messages/TcpMessage.h \
    utils/AppConfig.h \
    utils/time_provider/TimeProvider.h \
    utils/env_loader/EnvLoader.h

# Windows (WinPcap)
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
TRANSLATIONS +=
CONFIG += lrelease embed_translations

DISTFILES += \
    docs/git_guide.md \
    docs/doxygen_guide.md \
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

# Определение DESTDIR в зависимости от конфигурации
CONFIG(debug, debug|release) {
    DESTDIR = $$OUT_PWD/debug
} else {
    DESTDIR = $$OUT_PWD/release
}

# Копирование .env
win32 {
    QMAKE_POST_LINK += $$quote(cmd /c if not exist $$DESTDIR md $$DESTDIR & copy /Y $$shell_path($$PWD/.env) $$shell_path($$DESTDIR))
} else {
    QMAKE_POST_LINK += $$quote(mkdir -p $$DESTDIR && cp -f $$shell_path($$PWD/.env) $$shell_path($$DESTDIR))
}

# ===================================
# !!!!!! Очистка дубликатов !!!!!!
# ===================================
DISTFILES = $$unique(DISTFILES)
HEADERS = $$unique(HEADERS)
