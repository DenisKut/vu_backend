#include "TcpConnectionHandler.h"
#include <QDebug>

TcpConnectionHandler::TcpConnectionHandler(QTcpSocket *socket, QObject *parent)
	: QObject(parent), m_socket(socket)
{
	Q_ASSERT(m_socket);
	connect(m_socket, &QTcpSocket::readyRead, this, &TcpConnectionHandler::onReadyRead);
	connect(m_socket, &QTcpSocket::disconnected, this, &TcpConnectionHandler::onDisconnected);
}

TcpConnectionHandler::~TcpConnectionHandler()
{
	if (m_socket)
	{
		m_socket->deleteLater();
		m_socket = nullptr;
	}
}

QTcpSocket *TcpConnectionHandler::getSocket() const noexcept
{
	return m_socket;
}

void TcpConnectionHandler::onReadyRead()
{
	if (!m_socket)
	{
		qWarning("[TcpConnectionHandler] onReadyRead: m_socket is nullptr");
		return;
	}
	QByteArray data = m_socket->readAll();
	if (data.isEmpty())
	{
		qWarning("[TcpConnectionHandler] onReadyRead: received empty data");
		return;
	}
	m_buffer.append(data);
	emit messageReceived(m_buffer); // Сигнал отдаёт полный буфер
	m_buffer.clear();
}

void TcpConnectionHandler::onDisconnected()
{
	emit finished();
}

void TcpConnectionHandler::sendData(const QByteArray &data)
{
	if (!m_socket)
	{
		qWarning("[TcpConnectionHandler] sendData: m_socket is nullptr");
		return;
	}
	if (m_socket->state() != QAbstractSocket::ConnectedState)
	{
		qWarning("[TcpConnectionHandler] sendData: socket not connected");
		return;
	}
//	qint64 written = m_socket->write(data);
//    if (written != data.size())
//    {
//        qWarning("[TcpConnectionHandler] sendData: not all data written (%lld/%d)",
//                 written, data.size());
//    }

    // Отправка в т.ч. больших данных (фрагментированных)
    qint64 written = 0;
    while (written < data.size()) {
        const qint64 result = m_socket->write(data.constData() + written,
                                              data.size()      - written);
        if (result <= 0) {
            qWarning() << "[TcpConnectionHandler] sendData: failed to write data:"
                       << m_socket->errorString();
            break;
        }
        written += result;
    }
}
