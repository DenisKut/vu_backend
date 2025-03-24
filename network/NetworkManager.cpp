#include "NetworkManager.h"
#include "handlers/TcpHandler.h"
#include "handlers/UdpHandler.h"
// #include "handlers/EthHandler.h"

NetworkManager::NetworkManager(TimeProvider* timeProvider, QObject* parent)
    : QObject(parent),
    m_timeProvider(timeProvider)
{
    // Инициализация обработчиков и перемещение в потоки
    m_tcpHandler.reset(new TcpHandler(m_timeProvider));
    m_tcpHandler->moveToThread(&m_tcpThread);

    m_udpHandler.reset(new UdpHandler(m_timeProvider));
    m_udpHandler->moveToThread(&m_udpThread);

    // m_ethHandler.reset(new EthHandler(m_timeProvider));
    // m_ethHandler->moveToThread(&m_ethThread);

    // Подключение сигналов от обработчиков
    connect(m_tcpHandler.data(), &TcpHandler::messageReceived,
            this, &NetworkManager::tcpMessageReceived);
    connect(m_udpHandler.data(), &UdpHandler::datagramReceived,
            this, &NetworkManager::udpDatagramReceived);
    // connect(m_ethHandler.data(), &EthHandler::frameReceived,
    //         this, &NetworkManager::ethFrameReceived);
}

NetworkManager::~NetworkManager() {
    stop();
}

void NetworkManager::start() {
    // Запуск потоков
    m_tcpThread.start();
    m_udpThread.start();
    m_ethThread.start();

    // Инициализация обработчиков через сигналы (потокобезопасно)
    QMetaObject::invokeMethod(m_tcpHandler.data(), "start", Qt::QueuedConnection);
    QMetaObject::invokeMethod(m_udpHandler.data(), "startListening", Qt::QueuedConnection, Q_ARG(quint16, 12345));
    // QMetaObject::invokeMethod(m_ethHandler.data(), "start", Qt::QueuedConnection);
}

void NetworkManager::stop() {
    // Остановка потоков
    m_tcpThread.quit();
    m_udpThread.quit();
    m_ethThread.quit();

    m_tcpThread.wait();
    m_udpThread.wait();
    m_ethThread.wait();
}

void NetworkManager::tcpMessageReceived(const TcpMessage &msg)
{

}

void NetworkManager::udpDatagramReceived(const Message &msg)
{

}
