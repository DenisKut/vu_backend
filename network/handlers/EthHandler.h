#ifndef ETHHANDLER_H
#define ETHHANDLER_H

#include <QObject>
#include <QCoreApplication>
#include <QByteArray>
#include <QDataStream>
#include <QTimer>
#include <QPointer>
#include "network/messages/RmoMessage.h"



#ifndef ETH_TYPE
#define ETH_TYPE 0x2020
#endif

#ifdef _WIN32
#include <pcap.h>
#include <windows.h>
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

#include <linux/filter.h>
#include <linux/if_ether.h>
#endif

#define TO_HEX(T,len) (QByteArray::fromRawData(reinterpret_cast<const char*>(T),len).toHex(' '))
#define TO_HEX_STR(T,len) (QString::fromLatin1(QByteArray::fromRawData(reinterpret_cast<const char*>(T), len).toHex(' ').toUpper()))

class EthHandler : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(EthHandler)
public:
    /**
     * @brief Конструктор обработчика Ethernet
     */
    explicit EthHandler(QObject* parent = nullptr);
    ~EthHandler() override;

    Q_INVOKABLE void initialize();
    void connectAdapter();
    Q_INVOKABLE void disconnectAdapter();
    Q_INVOKABLE void freeRes();
    Q_INVOKABLE bool sendMessage(const QByteArray &data);

    /**
     * @brief Разобрать входящий Ethernet-фрейм
     */
    void processIncomingFrame(const QByteArray& data);
    /**
     * @brief Сформировать и отправить кастомный Ethernet-фрейм
     */
    void sendCustomFrame(const QByteArray& payload);

    // ППУ:
    void calculateCoefficientsPPU(double angleTarget, double velocityTarget,
                                  double angleCurrent, double* angleLast,
                                  double* accelerationLast, double accelerationMax,
                                  double KP, double KD, double dt,
                                  double* C0, double* C1, double* C2);

public slots:
    void start();
    void checkReceived();

signals:
    /**
     * @brief Сигнал о приёме Ethernet-фрейма
     */
    void messageReceived(const QByteArray& msg);
    /**
     * @brief Сигнал о готовности сообщения RMO TargetPPU
     */
    void rmoTargetPpuReady(const RmoMessage_TargetPPU& msg);
#ifdef _WIN32
    void ethTimerReceiveRestart();
#endif

private:
    bool m_adapterOpened = false;
    void startReconnectTimer();
    void stopReconnectTimer();
    void attemptReconnect();
    QPointer<QTimer> m_reconnectTimer;
    int m_reconnectAttempts = 0;
    const int MAX_RECONNECT_ATTEMPTS = 5;
    const int RECONNECT_INTERVAL_MS = 800;
    QDateTime m_lastPacketTime;
    const int PACKET_TIMEOUT_MS = 2000;
#ifdef _WIN32
    pcap_if_t *pcapAllDevs = nullptr;
    pcap_t *pcapTx = nullptr;
    pcap_t *pcapRx = nullptr;
    QPointer<QThread> m_pollThread;
    std::atomic<bool> m_polling{false};
    bool setupCustomProtocolFilter(pcap_t* handle, u_int customEtherType);
    u_int m_customEtherType = ETH_TYPE;
    bool isReceivedPacket(pcap_t *testHandle);
    bool adapterCheckTypeReceived(pcap_if_t *d);
    pcap_if_t* findAdapters(pcap_if_t *pcapAllDevs);
    void startPollingThread();
    void stopPollingThread();
#else
    int ethSocketID = -1;
    char ifName[IFNAMSIZ] = {0};
    QPointer<QSocketNotifier> m_notifier;
    bool isEthernetInterface(const QNetworkInterface &interface);
    QString findWiredInterface();

    bool attachBpfFilter(int sockfd, uint16_t etherType);
#endif
};

#endif // ETHHANDLER_H
