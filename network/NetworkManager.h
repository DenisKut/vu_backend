#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QTimer>
#include "handlers/TcpHandler.h"
#include "handlers/UdpHandler.h"
#include "handlers/EthHandler.h"
#include "network/messages/UdpMessage.h"
#include "network/messages/TcpMessage.h"


class NetworkManager : public QObject {
    Q_OBJECT
public:
    explicit NetworkManager(QObject* parent = nullptr);
    ~NetworkManager();

    void start();
    void stop();

	public slots:
    void onTcpReceived(const TcpHeader& msg);
    void onUdpReceived(const QByteArray& msg);
    void onUdpReceivedDebug(const UdpMessage& msg);
    void onEthReceived(const QByteArray &msg);
    // Потокобезопасная отправка UDP
    void sendUdp(const QByteArray& data);
    // Потокобезопасная отправка ETH
    void sendEth(const QByteArray& data);
    // Потокобезопасная отправка TCP (для RMO)
    void sendTcpRmo(QSharedPointer<RmoBaseMessage> msg);

private:
    QTimer m_ethTimer;

    TcpHandler* m_tcpHandler;
    UdpHandler* m_udpHandler;
    EthHandler* m_ethHandler;

    QThread* m_tcpThread;
    QThread* m_udpThread;
    QThread* m_ethThread;
};

#endif // NETWORKMANAGER_H
