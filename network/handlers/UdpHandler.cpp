#include "UdpHandler.h"
#include "utils/AppConfig.h"
#include <QNetworkDatagram>

UdpHandler::UdpHandler(QObject* parent)
    : QObject(parent), m_socket(nullptr) {}

UdpHandler::~UdpHandler() { stop(); }

void UdpHandler::start(int port) {
    if (m_socket) return;
    m_port = port;
    m_socket = new QUdpSocket(this);
    m_socket->bind(QHostAddress::Any, m_port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    connect(m_socket, &QUdpSocket::readyRead, this, &UdpHandler::onReadyRead);
}

void UdpHandler::stop() {
    if (m_socket) {
        m_socket->close();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
}

void UdpHandler::sendResponse(const QByteArray& data,
                              const QHostAddress& addr,
                              quint16 port) {
    if (m_socket) m_socket->writeDatagram(data, addr, port);
}

void UdpHandler::onReadyRead() {
    while (m_socket && m_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(int(m_socket->pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort;
        m_socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        emit packetReceived(datagram, sender, senderPort);
    }
}