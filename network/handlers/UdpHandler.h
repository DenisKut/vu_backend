#ifndef UDPHANDLER_H
#define UDPHANDLER_H

#include <QObject>
#include <QUdpSocket>
#include "../messages/Message.h"     // Убедитесь, что файл существует
#include "utils/TimeProvider.h"   // Исправлен путь

class UdpHandler : public QObject {
    Q_OBJECT
public:
    explicit UdpHandler(TimeProvider* timeProvider, QObject* parent = nullptr);
    ~UdpHandler();

public slots:
    void startListening(quint16 port);
    void sendDatagram(const QHostAddress& address, quint16 port, const QByteArray& data);

signals:
    void datagramReceived(const Message& message);
    void errorOccurred(const QString& error);

private slots:
    void onReadyRead();

private:
    TimeProvider* m_timeProvider;
    QUdpSocket* m_socket;
};

#endif // UDPHANDLER_H
