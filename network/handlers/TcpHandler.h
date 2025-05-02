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
    explicit TcpHandler(TimeProvider* timeProvider);
    ~TcpHandler();

    void disconnect();

    void sendMessageRMO(const QByteArray &data);
    void sendMessageResult(const QByteArray &data);

signals:
    void messageReceived(const TcpHeader& msg);
    void errorOccurred(const QString& error);

public slots:
    void incomingConnection(); // Приём запросов от клиентов на подключение
    void reconnect(); // Новый слот для переподключения
    void connectToHost(const QString& host, quint16 port);
    void startTimers();
    void stopTimers();

private slots:
    void initialize();
    void onReadyRead();
    void onReadyReadRMO();
    void onConnected();
    void onDisconnected();
    void onDisconnectedRMO();
    void onError(QAbstractSocket::SocketError error);
    void sendPeriodicMessages();
    void handleRegistration();

private:

    TimeProvider* m_timeProvider = nullptr;
    QTcpSocket* m_socket = nullptr;
    QTcpSocket* m_socketRMO = nullptr;
    QTcpServer* m_serverRMO = nullptr;
    QTimer* m_registrationTimer = nullptr;
    QTimer* m_periodicTimer = nullptr;
    QTimer* m_reconnectTimer = nullptr;

    QByteArray m_buffer;
    QByteArray m_bufferRMO;

    QString m_host;
    quint16 m_port;

    bool m_isRegistered = false;

    bool initServer();

    void sendMessage(const QByteArray &data);
    void processIncomingMessage(const QByteArray &data);

    /// Разбор сообщений, принятых от РМО
    void processIncomingMessageRMO(const QByteArray &data);

    template<typename F>
    void processBuffer(QByteArray& buffer, F processFunc);
};

#endif // TCPHANDLER_H
