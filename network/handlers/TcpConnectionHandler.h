#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QThread>
#include <QByteArray>
#include <QPointer>

class TcpConnectionHandler : public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY(TcpConnectionHandler)
public:
	/**
	 * @brief Создаёт обработчик TCP-соединения
	 * @param socket Владеемый QTcpSocket (не nullptr)
	 * @param parent Родительский QObject
	 */
	explicit TcpConnectionHandler(QTcpSocket *socket, QObject *parent = nullptr);
	~TcpConnectionHandler() override;

	/**
	 * @brief Отправить данные по TCP
	 * @param data Бинарные данные
	 */
	void sendData(const QByteArray &data);

	/**
	 * @brief Получить указатель на сокет (не nullptr)
	 */
	QTcpSocket *getSocket() const noexcept;

signals:
	/**
	 * @brief Сигнал о завершении работы соединения
	 */
	void finished();
	/**
	 * @brief Сигнал о получении данных (полный буфер)
	 */
	void messageReceived(const QByteArray &data);

public slots:
	void onReadyRead();
	void onDisconnected();

private:
	QPointer<QTcpSocket> m_socket;
	QByteArray m_buffer;
};