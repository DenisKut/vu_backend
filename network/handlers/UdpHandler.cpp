#include "UdpHandler.h"
#include <QNetworkDatagram>
#include <QMetaType>
#include <QDebug>

UdpHandler::UdpHandler(QObject* parent)
    : QObject(parent), m_socket(nullptr)
{
    // Регистрация QHostAddress для queued connections
    qRegisterMetaType<QHostAddress>("QHostAddress");
}

UdpHandler::~UdpHandler()
{
    stop();
}

void UdpHandler::start(const QString& ip, int port)
{
    if (m_socket) return;
    m_port = port;
    m_targetAddr = QHostAddress(ip);
    m_targetPort = port;
    m_socket = new QUdpSocket(this);
    bool bindResult = m_socket->bind(m_targetAddr, m_port,
                   QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    connect(m_socket, &QUdpSocket::readyRead, this, &UdpHandler::onReadyRead);
    if (bindResult) {
        qInfo() << "[UDP] Listening on" << ip << ":" << port;
    } else {
        qCritical() << "[UDP] Failed to bind on" << ip << ":" << port;
    }
}

void UdpHandler::stop()
{
    if (m_socket) {
        m_socket->close();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
}

void UdpHandler::sendMessage(const QByteArray& data)
{
    if (!m_socket) {
        qWarning("[UdpHandler] sendMessage: m_socket is nullptr");
        return;
    }
#if defined(QT_DEBUG) || defined(TRACE)
    QStringList traceParts;
#ifdef QT_DEBUG
    traceParts << "[UDP] Sending datagram";
#endif
#ifdef TRACE
    traceParts << "to" << m_targetAddr.toString() + ":" + QString::number(m_targetPort)
               << ", size:" + QString::number(data.size());
#endif
    qInfo().noquote() << traceParts.join(" ");
#endif
    qint64 written = m_socket->writeDatagram(data, m_targetAddr, m_targetPort);
    if (written != data.size()) {
        qWarning("[UdpHandler] sendMessage: not all data written (%lld/%d)", written, data.size());
    }
}

void UdpHandler::onReadyRead()
{
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
                                    quint16 senderPort)
{
    Q_UNUSED(data);
    Q_UNUSED(sender);
    Q_UNUSED(senderPort);
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
                                  quint16 port)
{
    Q_UNUSED(addr);
    Q_UNUSED(port);
    // Здесь формирование пакета (например, добавление заголовка НА БУДУЩЕЕ)
    QByteArray packet = payload; // Пример: без изменений
    sendMessage(packet);
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
