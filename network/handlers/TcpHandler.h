#ifndef TCPHANDLER_H
#define TCPHANDLER_H

#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>
#include <QDataStream>
#include <QTimer>
#include <QVector>
#include <QWebSocketServer>
#include <QWebSocket>
#include "../messages/TcpMessage.h"
#include "utils/time_provider/TimeProvider.h"
#include "core/logger/Logger.h"

class TcpHandler : public QObject {
    Q_OBJECT
public:
    explicit TcpHandler(QObject* parent = nullptr);
    ~TcpHandler();

    void disconnect();

    void sendMessageRMO(const QByteArray &data);
    void sendMessageResult(const QByteArray &data);

    // отправка WebSocket-клиентам
    void sendMessageRMOJson(const QJsonObject& obj);

signals:
    void messageReceived(const TcpHeader& msg);
    void errorOccurred(const QString& error);

public slots:
    void incomingConnection(); // TCP: Приём запросов от клиентов на подключение
    void reconnect();
    void connectToHost(const QString& host, quint16 port);
    void startTimers();
    void stopTimers();

private slots:
    void initialize();
    void onReadyRead();
//    void onReadyReadRMO();
    void onConnected();
    void onDisconnected();
//    void onDisconnectedRMO();
    void onError(QAbstractSocket::SocketError error);
    void sendPeriodicMessages();
    void handleRegistration();

    // WebSocket
    void onWebSocketNewConnection();
//    void onWebSocketDisconnected();
    void onWebSocketTextMessageReceived(const QString& message);

private:
    QTcpSocket* m_socket = nullptr;
    QTcpServer* m_serverRMO = nullptr;
    QVector<QTcpSocket*> m_rmoSockets; // Список всех TCP-клиентов РМО

    // WebSocket сервер и клиенты
    QWebSocketServer* m_wsServer = nullptr;
    QVector<QWebSocket*> m_wsClients;

    QTimer* m_registrationTimer = nullptr;
    QTimer* m_periodicTimer = nullptr;
    QTimer* m_reconnectTimer = nullptr;

    QByteArray m_buffer;
    QHash<QTcpSocket*, QByteArray> m_buffersRMO; // Буферы для каждого TCP-клиента РМО

    QString m_host;
    quint16 m_port;

    bool m_isRegistered = false;

    bool initServer();
    bool initWebSocketServer();

    void sendMessage(const QByteArray &data);
    void processIncomingMessage(const QByteArray &data);

    void processIncomingMessageRMO(const QByteArray &data, QTcpSocket* sender);

    template<typename F>
    void processBuffer(QByteArray& buffer, F processFunc);
};

#endif // TCPHANDLER_H
