#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QTimer>
#include "handlers/TcpHandler.h"
#include "handlers/UdpHandler.h"
#include "handlers/EthHandler.h"
#include "network/messages/Message.h"
#include "network/messages/TcpMessage.h"
#include "network/messages/EthMessage.h"
#include "utils/env_loader/Settings.h"

class NetworkManager : public QObject {
    Q_OBJECT
public:
    explicit NetworkManager(QObject* parent = nullptr);
    ~NetworkManager();

    void start();
    void stop();

    // ППУ:
    //----------------------------------------------------------
    void calculateCoefficientsPPU(double angleTarget,  double velocityTarget,
                                  double angleCurrent, double* angleLast,
                                  double* accelerationLast, double accelerationMax,
                                  // double angleMin, double velocityMin,
                                  double KP, double KD, /*double dtLast,*/ double dt,
                                  double* C0, double* C1, double* C2);
    //----------------------------------------------------------

	public slots:
    void onTcpReceived(const TcpHeader& msg);
    void onUdpReceived(const Message& msg);
    void onUdpReceivedDebug(const Message& msg);
    void onEthReceived(const QByteArray &msg);

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
