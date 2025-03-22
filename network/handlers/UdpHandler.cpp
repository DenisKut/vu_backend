#include "UdpHandler.h"
#include <QNetworkDatagram>

UdpHandler::UdpHandler(TimeProvider* timeProvider, QObject* parent)
    : QObject(parent),
    m_timeProvider(timeProvider),
    m_socket(new QUdpSocket(this))  // Инициализация сокета
{
    // Подключаем сигнал readyRead к слоту onReadyRead
    connect(m_socket, &QUdpSocket::readyRead, this, &UdpHandler::onReadyRead);
}

UdpHandler::~UdpHandler() {
    if (m_socket) {
        m_socket->close();
        delete m_socket;
    }
}

void UdpHandler::startListening(quint16 port) {
    if (m_socket->bind(port, QUdpSocket::ShareAddress)) {
        qDebug() << "UDP listening on port" << port;
    } else {
        qWarning() << "Failed to bind UDP port:" << m_socket->errorString();
    }
}

void UdpHandler::sendDatagram(const QHostAddress& address, quint16 port, const QByteArray& data) {
    if (m_socket->writeDatagram(data, address, port) == -1) {
        qWarning() << "Failed to send UDP datagram:" << m_socket->errorString();
    }
}

void UdpHandler::onReadyRead() {
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        Message message;
        message.timestamp = m_timeProvider->currentTimestamp();
        message.payload = datagram.data();
        emit datagramReceived(message);  // Отправляем сообщение наружу
    }
}
