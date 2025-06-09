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
#include <QPointer>
#include "TcpConnectionHandler.h"
#include "../messages/TcpMessage.h"
#include "../messages/RmoMessage.h"

using json = nlohmann::json;

/**
 * @brief The TcpHandler class
 * Обработчик TCP-соединения
 */
class TcpHandler : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(TcpHandler)
public:
    /**
     * @brief Конструктор обработчика TCP
     */
    explicit TcpHandler(QObject* parent = nullptr);
    ~TcpHandler() override;

    Q_INVOKABLE void disconnect();

    Q_INVOKABLE void sendRmoMessage(int idx, QSharedPointer<RmoBaseMessage> msg); // по индексу
    Q_INVOKABLE void sendRmoMessageToAll(QSharedPointer<RmoBaseMessage> msg);     // всем TCP-RMO

    Q_INVOKABLE void sendRmoMessageJSON(QWebSocket* ws, QSharedPointer<RmoBaseMessage> msg);
    Q_INVOKABLE void sendRmoMessageToAllJSON(QSharedPointer<RmoBaseMessage> msg);

    void setEthSendCallback(std::function<void(const QByteArray&)> cb);
    void setUdpSendCallback(std::function<void(const QByteArray&)> cb);

signals:
    /**
     * @brief Сигнал о приёме TCP-сообщения
     */
    void messageReceived(const TcpHeader& msg);
    void errorOccurred(const QString& error);
    // --- MCFOS control signals ---
    void mcfosWriteRequested(QSharedPointer<RmoBaseMessage> msg);
    void mcfosStartRequested(QSharedPointer<RmoBaseMessage> msg);

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
    QPointer<QTcpSocket> m_socket;
    QPointer<QTcpServer> m_serverRMO;
    QPointer<QThread> m_mainSocketThread;
    QPointer<TcpConnectionHandler> m_mainConnection;
//    QThread* m_rmoThread = nullptr; // Новый поток для всех RMO TCP-клиентов
    QVector<TcpConnectionHandler*> m_rmoConnections;
    QPointer<QWebSocketServer> m_wsServer;
    QVector<QWebSocket*> m_wsClients;

    QPointer<QTimer> m_registrationTimer;
    QPointer<QTimer> m_periodicTimer;
    QPointer<QTimer> m_reconnectTimer;

    QByteArray m_buffer;

    QString m_host;
    quint16 m_port = 0;

    bool m_isRegistered = false;

    bool initServer();
    bool initWebSocketServer();

    void sendMessage(const QByteArray &data);
    void processIncomingMessage(const QByteArray &data);

    void processIncomingMessageRMO(const QByteArray &data);

    template<typename F>
    void processBuffer(QByteArray& buffer, F processFunc);

    std::function<void(const QByteArray&)> m_ethSendCallback;
    std::function<void(const QByteArray&)> m_udpSendCallback;
};

#endif // TCPHANDLER_H
