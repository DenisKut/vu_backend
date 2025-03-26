#include "NetworkManager.h"
#include "handlers/TcpHandler.h"
#include "handlers/UdpHandler.h"
// #include "handlers/MacHandler.h"

NetworkManager::NetworkManager(TimeProvider* timeProvider, QObject* parent)
    : QObject(parent),
    m_timeProvider(timeProvider)
{
    // Инициализация обработчиков и перемещение в потоки
    m_tcpHandler.reset(new TcpHandler(m_timeProvider));
    m_tcpHandler->moveToThread(&m_tcpThread);

    m_udpHandler.reset(new UdpHandler(m_timeProvider));
    m_udpHandler->moveToThread(&m_udpThread);

    m_macHandler.reset(new MacHandler(m_timeProvider));
    m_macHandler->moveToThread(&m_macThread);

    // Таймер запуска обработчика принятых MAC-пакетов
    macTimerReceivePeriod = 1;                           // Период таймера = 1 мс
    macTimerReceive = new QTimer;
    macTimerReceive->setInterval(macTimerReceivePeriod); // Установка периода таймера
    macTimerReceive->setSingleShot(true);                // Автозапуск таймера
    // Таймер  запускается однократно, т.к. обработка MAC пакета может быть долгой

    // Подключение сигналов от обработчиков
    connect(m_tcpHandler.data(), &TcpHandler::messageReceived,
            this, &NetworkManager::tcpMessageReceived);
    connect(m_udpHandler.data(), &UdpHandler::datagramReceived,
            this, &NetworkManager::udpDatagramReceived);
    connect(macTimerReceive, &QTimer::timeout,
            m_macHandler.data(), &MacHandler::checkReceived);
    connect(m_macHandler.data(), &MacHandler::macTimerReceiveRestart,
            macTimerReceive, QOverload<>::of(&QTimer::start));
}

NetworkManager::~NetworkManager() {
    stop();
}

void NetworkManager::start() {
    // Запуск потоков
    m_tcpThread.start();
    m_udpThread.start();
    m_macThread.start();

    // Инициализация обработчиков через сигналы (потокобезопасно)
    QMetaObject::invokeMethod(m_tcpHandler.data(), "start", Qt::QueuedConnection);
    QMetaObject::invokeMethod(m_udpHandler.data(), "startListening", Qt::QueuedConnection, Q_ARG(quint16, 12345));
    // QMetaObject::invokeMethod(m_ethHandler.data(), "start", Qt::QueuedConnection);

    // Запуск таймера вызова обработчика принятых MAC-пакетов:
    macTimerReceive->start();
}

void NetworkManager::stop() {
    // Остановка потоков
    m_tcpThread.quit();
    m_udpThread.quit();
    m_macThread.quit();

    m_tcpThread.wait();
    m_udpThread.wait();
    m_macThread.wait();

    // Остановка таймера вызова обработчика принятых MAC-пакетов:
    macTimerReceive->stop();
}

void NetworkManager::tcpMessageReceived(const TcpMessage &msg)
{

}

void NetworkManager::udpDatagramReceived(const Message &msg)
{

}
