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
    explicit UdpHandler(TimeProvider* timeProvider,
                        QObject* parent = nullptr);

    ~UdpHandler();

    Q_INVOKABLE void start(int port);
    void stop();

signals:
    void packetReceived(const QByteArray& data,
                        const QHostAddress& sender,
                        quint16 senderPort);

public slots:
    void sendResponse(const QByteArray& data,
                      const QHostAddress& addr,
                      quint16 port);

private:
    void run();

    TimeProvider* m_timeProvider;
    quint16 m_port;
    std::thread m_thread;
    std::atomic<bool> m_running;
    QUdpSocket* m_socket = nullptr;
};
