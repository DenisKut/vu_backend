#ifndef TCPHANDLER_H
#define TCPHANDLER_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include "../messages/TcpMessage.h"
#include "utils/time_provider/TimeProvider.h"

class TcpHandler : public QObject {
    Q_OBJECT
public:
    explicit TcpHandler(TimeProvider* timeProvider, QObject* parent = nullptr);
    ~TcpHandler();

    void connectToHost(const QString& host, quint16 port);
    void disconnect();

signals:
    void messageReceived(const TcpHeader& msg);
    void errorOccurred(const QString& error);

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError error);
    void sendPeriodicMessages();
    void handleRegistration();

private:
    TimeProvider* m_timeProvider;
    QTcpSocket* m_socket;

    QTimer m_registrationTimer;
    QTimer m_periodicTimer;

    bool m_isRegistered = false;

    void sendMessage(const QByteArray &data);
    void processIncomingMessage(std::unique_ptr<TcpHeader>& parsed);
};

#endif // TCPHANDLER_H
