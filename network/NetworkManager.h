#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QTimer>
#include "handlers/TcpHandler.h"
#include "handlers/UdpHandler.h"
#include "handlers/MacHandler.h"
#include "utils/time_provider/TimeProvider.h"
#include "utils/AppConfig.h"

class NetworkManager : public QObject {
    Q_OBJECT
public:
    explicit NetworkManager(TimeProvider* timeProvider, QObject* parent = nullptr);
    ~NetworkManager();

    void start();
    void stop();

public slots:
    void onTcpReceived(const TcpMessage& msg);
    void onUdpReceived(const Message& msg);
    void onMacReceived(const Message& msg);

private:
    TimeProvider* m_timeProvider;
    QTimer m_macTimer;
    TcpHandler m_tcpHandler;
    UdpHandler m_udpHandler;
    MacHandler m_macHandler;
};

#endif // NETWORKMANAGER_H
