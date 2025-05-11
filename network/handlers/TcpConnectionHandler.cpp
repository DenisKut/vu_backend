#include "TcpConnectionHandler.h"

TcpConnectionHandler::TcpConnectionHandler(QTcpSocket* socket, QObject* parent)
    : QObject(parent), m_socket(socket)
{
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpConnectionHandler::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpConnectionHandler::onDisconnected);
}

TcpConnectionHandler::~TcpConnectionHandler() {
    if (m_socket) m_socket->deleteLater();
}

QTcpSocket* TcpConnectionHandler::getSocket() const { return m_socket; }

void TcpConnectionHandler::start() {
    // Можно добавить инициализацию, если нужно
}

void TcpConnectionHandler::onReadyRead() {
    m_buffer.append(m_socket->readAll());
    emit messageReceived(m_buffer);
    m_buffer.clear();
}

void TcpConnectionHandler::onDisconnected() {
    emit finished();
}

void TcpConnectionHandler::sendData(const QByteArray& data) {
    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->write(data);
    }
}
