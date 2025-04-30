#ifndef TCPHANDLER_H
#define TCPHANDLER_H

#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>
#include <QDataStream>
#include <QTimer>
#include "../messages/TcpMessage.h"
#include "utils/time_provider/TimeProvider.h"
#include "core/logger/Logger.h"

class TcpHandler : public QObject {
    Q_OBJECT
public:
    explicit TcpHandler(TimeProvider* timeProvider, QObject* parent = nullptr);
    ~TcpHandler();

    void connectToHost(const QString& host, quint16 port);
    void disconnect();

    void sendMessageRMO(const QByteArray &data);
    void sendMessageResult(const QByteArray &data);

signals:
    void messageReceived(const TcpHeader& msg);
    void errorOccurred(const QString& error);

public slots:
    void incomingConnection(); // Приём запросов от клиентов на подключение

private slots:
    void onReadyRead();
    void onReadyReadRMO();
    void onConnected();
    void onDisconnected();
    void onDisconnectedRMO();
    void onError(QAbstractSocket::SocketError error);
    void sendPeriodicMessages();
    void handleRegistration();

private:
    TimeProvider* m_timeProvider;
    QTcpSocket* m_socket;

    QTcpServer* m_serverRMO;
    QTcpSocket* m_socketRMO;

    QByteArray m_buffer;
    QByteArray m_bufferRMO;

    QTimer m_registrationTimer;
    QTimer m_periodicTimer;

    bool m_isRegistered = false;

    void sendMessage(const QByteArray &data);
    void processIncomingMessage(const QByteArray &data);

    /// Разбор сообщений, принятых от РМО
    void processIncomingMessageRMO(const QByteArray &data);

    template<typename F>
    void processBuffer(QByteArray& buffer, F processFunc);
};

#endif // TCPHANDLER_H
