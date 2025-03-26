#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

// Класс отвечает за подключения и обработку протокольных сообщений
// в потоках подключения
#include <QObject>
#include <QThread>
#include <QTimer>
#include <QScopedPointer>
#include "handlers/TcpHandler.h"
#include "handlers/UdpHandler.h"
#include "handlers/MacHandler.h"
#include "../utils/TimeProvider.h"

class NetworkManager : public QObject {
    Q_OBJECT
public:
    explicit NetworkManager(TimeProvider* timeProvider, QObject* parent = nullptr);
    ~NetworkManager();

    QTimer *macTimerReceive;    // Таймер запуска обработчика принятых MAC-пакетов
    int macTimerReceivePeriod;  // Период таймера обработчика принятых MAC-пакетов

    void start();    // Запуск всех обработчиков
    void stop();     // Остановка всех обработчиков

public slots:
    void tcpMessageReceived(const TcpMessage& msg); // Обработчик принятых TCP пакетов
    void udpDatagramReceived(const Message& msg);   // Обработчик принятых UDP пакетов
    // Обработчик принятых MAC пакетов находится в EthHandler
private:
    TimeProvider* m_timeProvider;

    // Потоки и обработчики
    QThread m_tcpThread;
    QThread m_udpThread;
    QThread m_macThread;

    QScopedPointer<TcpHandler> m_tcpHandler;
    QScopedPointer<UdpHandler> m_udpHandler;
    QScopedPointer<MacHandler> m_macHandler;
};

#endif // NETWORKMANAGER_H
