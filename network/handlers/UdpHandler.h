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
    Q_INVOKABLE void start(const QString& ip, int port);
    void stop();

    // Метод для разбора входящих пакетов
    void processIncomingDatagram(const QByteArray& data,
                                 const QHostAddress& sender,
                                 quint16 senderPort);

    // Метод для формирования и отправки пакета
    void sendCustomPacket(const QByteArray& payload,
                                const QHostAddress& addr,
                                quint16 port);

signals:
    void packetReceived(const QByteArray& data,
                        const QHostAddress& sender,
                        quint16 senderPort);

public slots:
    Q_INVOKABLE void sendMessage(const QByteArray& data,
                      const QHostAddress& addr,
                      quint16 port);

private slots:
    void onReadyRead();

private:
    quint16 m_port = 0;
    QUdpSocket* m_socket = nullptr;
};
