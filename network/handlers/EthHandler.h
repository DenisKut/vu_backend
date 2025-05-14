#ifndef ETHHANDLER_H
#define ETHHANDLER_H

#include <QObject>
#include <QCoreApplication>
#include <QByteArray>
#include <QDataStream>
#include "utils/time_provider/TimeProvider.h"

#include <chrono>

#ifdef _WIN32
#include <pcap.h>
#include <atomic>
#include <QThread>
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

#ifdef _WIN32
    pcap_if_t *pcapAllDevs = nullptr;
    pcap_t *pcapTx = nullptr;
    pcap_t *pcapRx = nullptr;
	QThread* m_pollThread = nullptr;
    std::atomic<bool> m_polling{false};
#else
    int ethSocketID = -1;
    char ifName[IFNAMSIZ] = {0};
    QSocketNotifier* m_notifier = nullptr;
    bool isEthernetInterface(const QNetworkInterface &interface);
    QString findWiredInterface();
#endif
};

#endif // ETHHANDLER_H
