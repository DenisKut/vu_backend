#include "UdpHandler.h"
#include "utils/AppConfig.h"
#include <QNetworkDatagram>
#include <QMetaType>

UdpHandler::UdpHandler(QObject* parent)
    : QObject(parent), m_socket(nullptr) {
    // Регистрация QHostAddress для queued connections
    qRegisterMetaType<QHostAddress>("QHostAddress");
}

UdpHandler::~UdpHandler() { stop(); }

void UdpHandler::start(const QString& ip, int port) {
    if (m_socket) return;
    m_port = port;
    m_socket = new QUdpSocket();
    m_socket->setParent(this);        // теперь можно назначить родителя
    bool bindResult = m_socket->bind(QHostAddress(ip), m_port,
                   QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    connect(m_socket, &QUdpSocket::readyRead, this, &UdpHandler::onReadyRead);
    if (bindResult) {
        qInfo() << "[UDP] Listening on" << ip << ":" << port;
    } else {
        qCritical() << "[UDP] Failed to bind on" << ip << ":" << port;
    }
}

void UdpHandler::stop() {
    if (m_socket) {
        m_socket->close();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
}

void UdpHandler::sendMessage(const QByteArray& data,
                              const QHostAddress& addr,
                              quint16 port) {
    if (m_socket) {
        qInfo() << "[UDP] Sending datagram to" << addr.toString()
                << ":" << port
                << ", size:" << data.size();
        m_socket->writeDatagram(data, addr, port);
    }
}

void UdpHandler::onReadyRead() {
    while (m_socket && m_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(int(m_socket->pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort;
        m_socket->readDatagram(datagram.data(), datagram.size(),
                                                        &sender, &senderPort);
        qInfo() << "[UDP] Received datagram from" << sender.toString()
                << ":" << senderPort
                << ", size:" << datagram.size();
        processIncomingDatagram(datagram, sender, senderPort); // Вызов разбора
        emit packetReceived(datagram, sender, senderPort);
    }
}

void UdpHandler::processIncomingDatagram(const QByteArray& data,
                                    const QHostAddress& sender,
                                    quint16 senderPort) {
    // Здесь разбор пакета, логика обработки, парсинг и т.д.
    // Пример: просто hex :)
    qDebug() << "[UDP] Parsed datagram from" << sender.toString()
             << ":" << senderPort
             << ":" << data.toHex(' ').toUpper();
    // TODO: добавить реальный разбор
}

void UdpHandler::sendCustomPacket(const QByteArray& payload,
                                  const QHostAddress& addr,
                                  quint16 port) {
    // Здесь формирование пакета (например, добавление заголовка НА БУДУЩЕЕ)
    QByteArray packet = payload; // Пример: без изменений
    sendMessage(packet, addr, port);
    qDebug() << "[UDP] Custom packet sent to" << addr.toString()
             << ":" << port
             << ", size:" << packet.size();
}
