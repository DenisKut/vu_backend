#include "UdpHandler.h"
#include <QNetworkDatagram>

UdpHandler::UdpHandler(TimeProvider* timeProvider, QObject* parent)
    : QObject(parent), m_timeProvider(timeProvider), m_socket(new QUdpSocket(this))
{
    connect(m_socket, &QUdpSocket::readyRead, this, &UdpHandler::onReadyRead);
}

UdpHandler::~UdpHandler() {
    stopListening();
    delete m_socket;
}

void UdpHandler::startListening(quint16 port) {
    if (m_socket->state() != QAbstractSocket::BoundState) {
        if (!m_socket->bind(port, QUdpSocket::ShareAddress)) {
            emit errorOccurred(m_socket->errorString());
        }
    }
}

void UdpHandler::stopListening() {
    if (m_socket->state() == QAbstractSocket::BoundState) {
        m_socket->close();
    }
}

void UdpHandler::sendDatagram(const QHostAddress& address, quint16 port, const QByteArray& data) {
    m_socket->writeDatagram(data, address, port);
}

void UdpHandler::onReadyRead() {
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        Message msg;
        msg.timestamp = m_timeProvider->currentTimestamp();
        msg.payload = datagram.data();
        emit datagramReceived(msg);
    }
}