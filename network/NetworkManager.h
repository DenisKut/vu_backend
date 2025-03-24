#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

// Класс отвечает за подключения и обработку протокольных сообщений
// в потоках подключения
#include <QObject>
#include <QThread>
#include <QScopedPointer>
#include "handlers/TcpHandler.h"
#include "handlers/UdpHandler.h"
#include "handlers/EthHandler.h"
#include "../utils/TimeProvider.h"

class NetworkManager : public QObject {
    Q_OBJECT
public:
    explicit NetworkManager(TimeProvider* timeProvider, QObject* parent = nullptr);
    ~NetworkManager();

    void start();    // Запуск всех обработчиков
    void stop();     // Остановка всех обработчиков

signals:
    void tcpMessageReceived(const TcpMessage& msg);
    void udpDatagramReceived(const Message& msg);
    void ethFrameReceived(const Message& msg);

private:
    TimeProvider* m_timeProvider;

    // Потоки и обработчики
    QThread m_tcpThread;
    QThread m_udpThread;
    QThread m_ethThread;

    QScopedPointer<TcpHandler> m_tcpHandler;
    QScopedPointer<UdpHandler> m_udpHandler;
    QScopedPointer<EthHandler> m_ethHandler;
};

#endif // NETWORKMANAGER_H
