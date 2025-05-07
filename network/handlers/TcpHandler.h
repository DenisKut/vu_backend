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
#include <QJsonObject>
#include "TcpConnectionHandler.h"
#include "../messages/TcpMessage.h"
#include "../messages/RmoMessage.h"

using json = nlohmann::json;

class TcpHandler : public QObject {
    Q_OBJECT
public:
    explicit TcpHandler(QObject* parent = nullptr);
    ~TcpHandler();

    Q_INVOKABLE void disconnect();

    Q_INVOKABLE void sendRmoMessage(int idx, const RmoBaseMessage& msg); // по индексу
    Q_INVOKABLE void sendRmoMessageToAll(const RmoBaseMessage& msg);       // всем TCP-RMO

    Q_INVOKABLE void sendRmoMessageJSON(QWebSocket* ws, const RmoBaseMessage& msg);
    Q_INVOKABLE void sendRmoMessageToAllJSON(const RmoBaseMessage& msg);

signals:
    void messageReceived(const TcpHeader& msg);
    void errorOccurred(const QString& error);

public slots:
    void initialize();
    void incomingConnection();
    void reconnect();
    void connectToHost(const QString& host, quint16 port);
    void startTimers();
    void stopTimers();

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onRmoDisconnected(TcpConnectionHandler* handler);
    void onError(QAbstractSocket::SocketError error);
    void sendPeriodicMessages();
    void handleRegistration();

    // WebSocket
    void onWebSocketNewConnection();
    void onWebSocketTextMessageReceived(const QString& message, QWebSocket* ws);

private:
    QTcpSocket* m_socket = nullptr;
    QTcpServer* m_serverRMO = nullptr;
    QThread* m_mainSocketThread = nullptr;
    TcpConnectionHandler* m_mainConnection = nullptr;
//    QThread* m_rmoThread = nullptr; // Новый поток для всех RMO TCP-клиентов
    QVector<TcpConnectionHandler*> m_rmoConnections;
    QWebSocketServer* m_wsServer = nullptr;
    QVector<QWebSocket*> m_wsClients;

    QTimer* m_registrationTimer = nullptr;
    QTimer* m_periodicTimer = nullptr;
    QTimer* m_reconnectTimer = nullptr;

    QByteArray m_buffer;

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
