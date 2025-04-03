#ifndef TCPHANDLER_H
#define TCPHANDLER_H

#include <QObject>
#include <QTcpSocket>
#include <QDataStream>
#include "../messages/TcpMessage.h"
#include "utils/time_provider/TimeProvider.h"
#include "core/logger/Logger.h"

class TcpHandler : public QObject {
    Q_OBJECT
public:
    explicit TcpHandler(TimeProvider* timeProvider, QObject* parent = nullptr);
    ~TcpHandler();

public slots:
    void connectToHost(const QString& host, quint16 port);
    void sendMessage(const TcpHeader& msg);
    void disconnect();

signals:
    void messageReceived(const TcpHeader& msg);
    void errorOccurred(const QString& error);

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError error);

private:
    TimeProvider* m_timeProvider;
    QTcpSocket* m_socket;
};

#endif // TCPHANDLER_H
