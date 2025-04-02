#include "TcpHandler.h"
#include <QHostAddress>

TcpHandler::TcpHandler(TimeProvider* timeProvider, QObject* parent)
    : QObject(parent), m_timeProvider(timeProvider), m_socket(new QTcpSocket(this))
{
    // Настройка неблокирующего режима
    m_socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    
    connect(m_socket, &QTcpSocket::connected, this, &TcpHandler::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpHandler::onDisconnected);
//    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
//            this, &TcpHandler::onError);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpHandler::onReadyRead);
}

TcpHandler::~TcpHandler() {
    disconnect();
}

void TcpHandler::onConnected() {
    qDebug() << "Connected to TCP host";
}

void TcpHandler::onDisconnected() {
    qDebug() << "Disconnected from TCP host";
}

void TcpHandler::onError(QAbstractSocket::SocketError error) {
    if(error)
        emit errorOccurred(m_socket->errorString());
}

void TcpHandler::connectToHost(const QString& host, quint16 port) {
    m_socket->connectToHost(host, port, QIODevice::ReadWrite, QAbstractSocket::AnyIPProtocol);
}

void TcpHandler::sendMessage(const TcpHeader& msg) {
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream << msg.sizeData << msg.checkSum;
        m_socket->write(data);
    }
}

void TcpHandler::disconnect() {
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
    }
}

void TcpHandler::onReadyRead() {
    // while (m_socket->bytesAvailable() > 0) {
    //     QByteArray data = m_socket->readAll();
    //     TcpMessage msg;
    //     msg.timestamp = m_timeProvider->currentTimestamp();
    //     msg.payload = data;
    //     emit messageReceived(msg);
    // }
}
