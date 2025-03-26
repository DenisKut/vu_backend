QT += core network

TARGET = lun_vu
CONFIG += c++11 cmdline

#-------------------------------
# 1. Платформозависимые настройки
#-------------------------------

# Общие для всех платформ настройки
SOURCES += \
    core/CoreManager.cpp \
    core/Logger.cpp \
    main.cpp \
    network/NetworkManager.cpp \
    network/handlers/MacHandler.cpp \
    network/handlers/TcpHandler.cpp \
    network/handlers/UdpHandler.cpp \
    utils/TimeProvider.cpp

HEADERS += \
    core/CoreManager.h \
    core/Logger.h \
    network/NetworkManager.h \
    network/handlers/MacHandler.h \
    network/handlers/TcpHandler.h \
    network/handlers/UdpHandler.h \
    network/messages/Message.h \
    network/messages/TcpMessage.h \
    utils/TimeProvider.h

# Windows (WinPcap)
win32 {
    INCLUDEPATH += $$PWD/libs/wpdpack/Include
    LIBS += -L$$PWD/libs/wpdpack/Lib -lPacket -lwpcap

    # Для x64
    win32:contains(QMAKE_HOST.arch, x86_64) {
        LIBS += -L$$PWD/libs/wpdpack/Lib/x64 -lPacket -lwpcap
    }

    DEFINES += WPCAP HAVE_REMOTE
}

# Linux (libpcap)
linux {
    # Установить libpcap: `sudo apt-get install libpcap-dev`
    CONFIG += link_pkgconfig
    PKGCONFIG += libpcap

    # Или явное указание путей
    # INCLUDEPATH += /usr/include/pcap
    # LIBS += -lpcap
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
    libs/wpdpack/Lib/Packet.lib \
    readme.md

# Очистка дубликатов
DISTFILES = $$unique(DISTFILES)
HEADERS = $$unique(HEADERS)
