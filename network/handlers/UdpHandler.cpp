#include "UdpHandler.h"
#include <QNetworkDatagram>

UdpHandler::UdpHandler(TimeProvider* timeProvider, QObject* parent)
    : QObject(parent), m_timeProvider(timeProvider) {}

void UdpHandler::initialize() {
    if (!m_udpSocket) {
        m_udpSocket = new QUdpSocket(this);
        connect(m_udpSocket, &QUdpSocket::readyRead, this, &UdpHandler::onReadyRead);
    }
}

void UdpHandler::startListening(quint16 port) {
	if (!m_udpSocket || m_isBound) return;
    if (!m_udpSocket->bind(QHostAddress::Any, port)) {
        qWarning() << "Failed to bind UDP socket on port" << port;
    } else {
        qDebug() << "UDP socket bound on port" << port;
    }
}

void UdpHandler::stopListening() {
    if (m_udpSocket && m_isBound) {
        m_udpSocket->close();
        m_isBound = false;
    }
}

void UdpHandler::onReadyRead() {
    QList<QByteArray> datagrams;

    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpSocket->pendingDatagramSize());
        m_udpSocket->readDatagram(datagram.data(), datagram.size());
        datagrams.append(datagram);
    }

    for (const QByteArray& datagram : datagrams) {
        emit datagramReceived(datagram);
    }
}
