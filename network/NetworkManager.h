#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QTimer>
#include <QDebug>
#include "handlers/TcpHandler.h"
#include "handlers/UdpHandler.h"
#include "handlers/EthHandler.h"
#include "utils/time_provider/TimeProvider.h"
#include "core/logger/Logger.h"
#include "utils/AppConfig.h"

class NetworkManager : public QObject {
    Q_OBJECT
public:
    explicit NetworkManager(TimeProvider* timeProvider, QObject* parent = nullptr);
    ~NetworkManager();

    void start();
    void stop();

public slots:
    void onTcpReceived(const TcpHeader& msg);
    void onUdpReceived(const Message& msg);
    void onEthReceived(const QByteArray &msg);

private:
    TimeProvider* m_timeProvider;
    QTimer m_ethTimer;
    TcpHandler m_tcpHandler;
    UdpHandler m_udpHandler;
    EthHandler m_ethHandler;
};

#endif // NETWORKMANAGER_H
