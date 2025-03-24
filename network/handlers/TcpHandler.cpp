#include "TcpHandler.h"
#include <QHostAddress>

TcpHandler::TcpHandler(TimeProvider* timeProvider, QObject* parent)
    : QObject(parent), m_timeProvider(timeProvider), m_socket(new QTcpSocket(this))
{
    connect(m_socket, &QTcpSocket::connected, this, &TcpHandler::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpHandler::onDisconnected);
//    connect(m_socket, &QTcpSocket::errorOccurred,
//            this, &TcpHandler::onError);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpHandler::onReadyRead);
}

TcpHandler::~TcpHandler() {
    if (m_socket->isOpen()) {
        m_socket->close();
    }
}

// Реализация недостающих методов
void TcpHandler::onConnected() {
    qDebug() << "Connected to host";
}

void TcpHandler::onDisconnected() {
    qDebug() << "Disconnected from host";
}

void TcpHandler::onError(QAbstractSocket::SocketError error) {
    Q_UNUSED(error);  // Если параметр не используется
    emit errorOccurred(m_socket->errorString());
}

void TcpHandler::disconnect() {
    m_socket->disconnectFromHost();
}

void TcpHandler::connectToHost(const QString& host, quint16 port) {
    m_socket->connectToHost(host, port);
}

void TcpHandler::sendMessage(const TcpMessage& msg) {
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        QByteArray data;
        // Сериализация сообщения (пример)
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream << msg.timestamp << msg.payload;
        m_socket->write(data);
    }
}

void TcpHandler::onReadyRead() {
    QByteArray data = m_socket->readAll();
    TcpMessage msg;
    msg.timestamp = m_timeProvider->currentTimestamp();
    msg.payload = data;
    emit messageReceived(msg);
}
