#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QTimer>
#include "handlers/TcpHandler.h"
#include "handlers/UdpHandler.h"
#include "handlers/MacHandler.h"
#include "utils/time_provider/TimeProvider.h"

class NetworkManager : public QObject {
    Q_OBJECT
public:
    explicit NetworkManager(TimeProvider* timeProvider, QObject* parent = nullptr);
    ~NetworkManager();

    void start();
    void stop();

public slots:
    void onTcpMessage(const TcpMessage& msg);
    void onUdpDatagram(const Message& msg);

private:
    TimeProvider* m_timeProvider;
    QTimer m_macTimer;
    TcpHandler m_tcpHandler;
    UdpHandler m_udpHandler;
    MacHandler m_macHandler;
};

#endif // NETWORKMANAGER_H
