#ifndef TCPHANDLER_H
#define TCPHANDLER_H

#include <QObject>
#include <QTcpSocket>
#include <QDataStream>
#include "../messages/TcpMessage.h"
#include "utils/TimeProvider.h"

class TcpHandler : public QObject {
    Q_OBJECT

public:
    explicit TcpHandler(TimeProvider* timeProvider, QObject* parent = nullptr);
    ~TcpHandler();

public slots:
    void connectToHost(const QString& host, quint16 port);
    void sendMessage(const TcpMessage& msg);
    void disconnect();  // Переименуем в closeConnection для ясности

signals:
    void messageReceived(const TcpMessage& msg);
    void errorOccurred(const QString& error);

private slots:
    void onReadyRead();
    void onConnected();       // Добавляем реализацию
    void onDisconnected();    // Добавляем реализацию
    void onError(QAbstractSocket::SocketError error);

private:
    TimeProvider* m_timeProvider;
    QTcpSocket* m_socket;
};

#endif // TCPHANDLER_H
