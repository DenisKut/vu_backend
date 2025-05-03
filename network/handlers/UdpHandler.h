#ifndef UDPHANDLER_H
#define UDPHANDLER_H

#include <QObject>
#include <QUdpSocket>
#include "utils/time_provider/TimeProvider.h"

class UdpHandler : public QObject {
    Q_OBJECT

public:
    explicit UdpHandler(TimeProvider* timeProvider, QObject* parent = nullptr);
    Q_INVOKABLE void initialize();
    Q_INVOKABLE void startListening(quint16 port);
    Q_INVOKABLE void stopListening();

signals:
    void datagramReceived(const QByteArray& datagram);

private slots:
    void onReadyRead();

private:
    TimeProvider* m_timeProvider;
    QUdpSocket* m_udpSocket = nullptr;
    bool m_isBound = false;
};

#endif // UDPHANDLER_H
