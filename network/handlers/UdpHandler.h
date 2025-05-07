#pragma once

#include <QObject>
#include <QHostAddress>
#include <QUdpSocket>
#include <atomic>
#include <thread>
#include "utils/time_provider/TimeProvider.h"

class UdpHandler : public QObject {
    Q_OBJECT
public:
    explicit UdpHandler(QObject* parent = nullptr);
    ~UdpHandler();

    Q_INVOKABLE void start(int port);
    void stop();

signals:
    void packetReceived(const QByteArray& data,
                        const QHostAddress& sender,
                        quint16 senderPort);

public slots:
    Q_INVOKABLE void sendResponse(const QByteArray& data,
                      const QHostAddress& addr,
                      quint16 port);

private slots:
    void onReadyRead();

private:
    quint16 m_port = 0;
    QUdpSocket* m_socket = nullptr;
};