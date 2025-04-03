#ifndef MACHANDLER_H
#define MACHANDLER_H

#include <QTimer>
#include <QObject>
#include <QByteArray>
#include <QDebug>
#include <QDataStream>
#include "../messages/Message.h"
#include "utils/time_provider/TimeProvider.h"
#include "core/logger/Logger.h"
#include "utils/AppConfig.h"

#include <chrono>

// Библиотека WinPCap для работы с
// Ethernet raw data под Windows:
#ifdef _WIN32
#include <pcap.h>
#else
// Заголовочные файлы для работы с
// Ethernet raw data под Linux:
#include <QSocketNotifier>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <unistd.h>
#endif

class MacHandler : public QObject {
    Q_OBJECT

public:
    explicit MacHandler(TimeProvider* timeProvider, QObject* parent = nullptr);
    ~MacHandler();

    // Адаптеры FastEthernet:
    //---------------------------------------------------------------------
#ifdef _WIN32
    pcap_if_t *pcapAllDevs;                 // Список доступных адаптеров
    pcap_t *pcapTx;                         // Передатчик
    pcap_t *pcapRx;                         // Приёмник
#else
    int macSocketID;
    char ifName[IFNAMSIZ];
    void print_mac(const uint8_t* mac);
    void print_hex_dump(const uint8_t* buf, size_t len);
    std::string getTimestampStr();
#endif
    quint64 getTimestamp();

    void connectAdapter();                  // Подключиться к адаптеру Fast Ethernet под номером inum в списке адаптеров
    void disconnectAdapter();               // Отключиться от адаптера Fast Ethernet
    //---------------------------------------------------------------------

public slots:
    void start();
    void checkReceived(); // Проверка сокета на наличие принятых MAC пакетов

signals:
    void messageReceived(const Message& msg);

#ifdef _WIN32
signals:
    void macTimerReceiveRestart(); // Запуск таймера вызова обработчика принятых MAC пакетов
#else
private:
    QSocketNotifier* m_notifier = nullptr;
#endif

private:
    TimeProvider* m_timeProvider;
};

#endif // MACHANDLER_H
