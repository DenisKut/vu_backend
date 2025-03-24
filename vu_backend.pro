QT += core network

TARGET = lun_vu

CONFIG += c++11 cmdline

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        core/CoreManager.cpp \
        main.cpp \
        network/NetworkManager.cpp \
        network/handlers/EthHandler.cpp \
        network/handlers/TcpHandler.cpp \
        network/handlers/UdpHandler.cpp \
        utils/TimeProvider.cpp

TRANSLATIONS +=
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    docs/git_guide.md \
    readme.md

HEADERS += \
    core/CoreManager.h \
    network/NetworkManager.h \
    network/handlers/EthHandler.h \
    network/handlers/TcpHandler.h \
    network/handlers/UdpHandler.h \
    network/messages/Message.h \
    network/messages/TcpMessage.h \
    utils/TimeProvider.h
