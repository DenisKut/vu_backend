#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QThread>
#include <QByteArray>

class TcpConnectionHandler : public QObject {
    Q_OBJECT
public:
    explicit TcpConnectionHandler(QTcpSocket* socket, QObject* parent = nullptr);
    ~TcpConnectionHandler();

    void sendData(const QByteArray& data);
    QTcpSocket* getSocket() const;

signals:
    void finished();
    void messageReceived(const QByteArray& data);

public slots:
    void start();
    void onReadyRead();
    void onDisconnected();

private:
    QTcpSocket* m_socket;
    QByteArray m_buffer;
};