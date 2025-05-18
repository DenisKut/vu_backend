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
#if defined(QT_DEBUG) || defined(TRACE)
    QStringList traceParts;
#ifdef QT_DEBUG
    traceParts << "[UDP] Sending datagram";
#endif
#ifdef TRACE
    traceParts << "to" << addr.toString() + ":" + QString::number(port)
               << ", size:" + QString::number(data.size());
#endif
    qInfo().noquote() << traceParts.join(" ");
#endif
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
#if defined(QT_DEBUG) || defined(TRACE)
    QString logLine;
#ifdef QT_DEBUG
    logLine += "[UDP] Received datagram ";
#endif
#ifdef TRACE
    logLine += "from " + sender.toString()
            + ":" + QString::number(senderPort)
            + ", size:" + QString::number(datagram.size());
#endif
    qInfo().noquote() << logLine;
#endif
        processIncomingDatagram(datagram, sender, senderPort); // Вызов разбора
        emit packetReceived(datagram, sender, senderPort);
    }
}

void UdpHandler::processIncomingDatagram(const QByteArray& data,
                                    const QHostAddress& sender,
                                    quint16 senderPort) {
    // Здесь разбор пакета, логика обработки, парсинг и т.д.
    // Пример: просто hex :)
#if defined(QT_DEBUG) || defined(TRACE)
    QStringList traceParts;
#ifdef QT_DEBUG
    traceParts << "[UDP] Parsed datagram";
#endif
#ifdef TRACE
    traceParts << "from" << sender.toString() + ":" + QString::number(senderPort)
               << ":" + data.toHex(' ').toUpper();
#endif
    qDebug().noquote() << traceParts.join(" ");
#endif
    // TODO: добавить реальный разбор

}

void UdpHandler::sendCustomPacket(const QByteArray& payload,
                                  const QHostAddress& addr,
                                  quint16 port) {
    // Здесь формирование пакета (например, добавление заголовка НА БУДУЩЕЕ)
    QByteArray packet = payload; // Пример: без изменений
    sendMessage(packet, addr, port);
#if defined(QT_DEBUG) || defined(TRACE)
    QStringList traceParts;
#ifdef QT_DEBUG
    traceParts << "[UDP] Custom packet sent";
#endif
#ifdef TRACE
    traceParts << "to" << addr.toString() + ":" + QString::number(port)
               << ", size:" + QString::number(packet.size());
#endif
    qDebug().noquote() << traceParts.join(" ");
#endif

}
