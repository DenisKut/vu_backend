#ifndef ETHHANDLER_H
#define ETHHANDLER_H

#include <QObject>
#include <QCoreApplication>
#include <QByteArray>
#include <QDataStream>
#include <QTimer>
#include "utils/time_provider/TimeProvider.h"

#include <chrono>

#ifdef _WIN32
#include <pcap.h>
#include <atomic>
#include <QThread>

#ifndef PCAP_NETMASK_UNKNOWN
#define PCAP_NETMASK_UNKNOWN 0xFFFFFFFF
#endif

#else

#include <QSocketNotifier>
#include <QNetworkInterface>
#include <QFile>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <unistd.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <netinet/ether.h>
#endif

class EthHandler : public QObject {
    Q_OBJECT

public:
    explicit EthHandler(QObject* parent = nullptr);
    ~EthHandler();

    Q_INVOKABLE void initialize();
    void connectAdapter();
    Q_INVOKABLE void disconnectAdapter();
    //ADD MY BEG
    Q_INVOKABLE void freeRes();
    //ADD MY END
    Q_INVOKABLE bool sendMessage(const QByteArray &data);

public slots:
    void start();
    void checkReceived();

signals:
    void messageReceived(const QByteArray& msg);

#ifdef _WIN32
    void ethTimerReceiveRestart();
#endif

private:
    bool m_adapterOpened = false; // Новый флаг

//add 16.05 BEG
    void startReconnectTimer();
    void stopReconnectTimer();
    void attemptReconnect();
    QTimer* m_reconnectTimer = nullptr;
    int m_reconnectAttempts = 0;
    const int MAX_RECONNECT_ATTEMPTS = 5;
    const int RECONNECT_INTERVAL_MS = 800; // 3 секунды
    QDateTime m_lastPacketTime;
    const int PACKET_TIMEOUT_MS = 2000; // 10 секунд без пакетов
// add 16.05 END

#ifdef _WIN32
    pcap_if_t *pcapAllDevs = nullptr;
    pcap_t *pcapTx = nullptr;
    pcap_t *pcapRx = nullptr;
	QThread* m_pollThread = nullptr;
    std::atomic<bool> m_polling{false};

//DS ADD BEG


#ifndef ETH_TYPE
#define ETH_TYPE 0x2020
#endif
    bool setupCustomProtocolFilter(pcap_t* handle, u_int customEtherType);
    u_int m_customEtherType = ETH_TYPE;

    //MY ADD BEG
    bool isReceivedPacket(pcap_t *testHandle);
    bool adapterCheckTypeReceived(pcap_if_t *d);
    pcap_if_t* findAdapters(pcap_if_t *pcapAllDevs);



    //MY ADD END
//DS ADD END

//ADD 16.05 BEG
/*
    void startReconnectTimer();
    void stopReconnectTimer();
    void attemptReconnect();
    QTimer* m_reconnectTimer = nullptr;
    int m_reconnectAttempts = 0;
    const int MAX_RECONNECT_ATTEMPTS = 5;
    const int RECONNECT_INTERVAL_MS = 800; // 3 секунды
    QDateTime m_lastPacketTime;
    const int PACKET_TIMEOUT_MS = 2000; // 10 секунд без пакетов
*/

    void startPollingThread();
    void stopPollingThread();

//ADD 16.05 END
#else
    int ethSocketID = -1;
    char ifName[IFNAMSIZ] = {0};
    QSocketNotifier* m_notifier = nullptr;
    bool isEthernetInterface(const QNetworkInterface &interface);
    QString findWiredInterface();
#endif
};

#endif // ETHHANDLER_H
