#include "NetworkManager.h"
#include "utils/env_loader/Settings.h"

NetworkManager::NetworkManager(QObject* parent)
    : QObject(parent),
      m_tcpHandler(new TcpHandler()),
      m_udpHandler(new UdpHandler()),
      m_ethHandler(new EthHandler()),
      m_tcpThread(new QThread(this)),
      m_udpThread(new QThread(this)),
      m_ethThread(new QThread(this)) {

    // Перемещение обработчиков в потоки
    m_tcpHandler->moveToThread(m_tcpThread);
    m_udpHandler->moveToThread(m_udpThread);
    m_ethHandler->moveToThread(m_ethThread);

    // Подключение сигналов и слотов для потоков
    connect(m_tcpThread, &QThread::started, m_tcpHandler,
                            &TcpHandler::initialize);
    connect(m_ethThread, &QThread::started, m_ethHandler,
                            &EthHandler::initialize);

    // Подключение сигналов и слотов для обработчиков
    connect(m_tcpHandler, &TcpHandler::messageReceived, this,
                                    &NetworkManager::onTcpReceived,
                                    Qt::QueuedConnection);

#ifdef _WIN32
    // connect(&m_ethTimer, &QTimer::timeout, m_ethHandler,
	//                                            &EthHandler::checkReceived);
#endif

    // Запуск потоков
    m_tcpThread->start();
    m_udpThread->start();
    m_ethThread->start();

    // Отправка тестового сообщения на МЦФОС - РАБОТАЕТ
    // Порядок данных - старшим байтом вперёд!!!
    //------------------------------------------------------------------------
    QByteArray dataTemp;
    uint32_t value = 64;
    dataTemp.append(value >> 0);
    dataTemp.append(value >> 8);
    dataTemp.append(value >> 16);
    dataTemp.append(value >> 24);
    for (int i = 0; i < 7; ++i) {
        value = 0;
        dataTemp.append(value >> 0);
        dataTemp.append(value >> 8);
        dataTemp.append(value >> 16);
        dataTemp.append(value >> 24);
    }
    QHostAddress addr = QHostAddress("192.168.2.10");
    int port = 49498;
    QMetaObject::invokeMethod(m_udpHandler, [this, dataTemp, addr, port]() {
        m_udpHandler->sendMessage(dataTemp, addr, port);
    }, Qt::QueuedConnection);
    //------------------------------------------------------------------------
}

NetworkManager::~NetworkManager() {
    stop();

    // Завершение потоков
    m_tcpThread->quit();
    m_udpThread->quit();
    m_ethThread->quit();

    m_tcpThread->wait();
    m_udpThread->wait();
    m_ethThread->wait();

    // Удаление обработчиков
    m_tcpHandler->deleteLater();
    m_udpHandler->deleteLater();
    m_ethHandler->deleteLater();

    // Удаление потоков
    m_tcpThread->deleteLater();
    m_udpThread->deleteLater();
    m_ethThread->deleteLater();
}

void NetworkManager::start() {
    switch (currentMode())
    {
    case AppMode::VU_LOCALHOST:
        QMetaObject::invokeMethod(m_tcpHandler, [this]() {
            m_tcpHandler->connectToHost(
                appSettings().value("vu_settings/tcp_host_ip").toString(),
                appSettings().value("vu_settings/tcp_host_port").toInt());
        });
        QMetaObject::invokeMethod(m_udpHandler, [this]() {
            QString ip = appSettings().value("vu_settings/udp_ip", "0.0.0.0")
                                                                .toString();
            int port = appSettings().value("vu_settings/udp_port").toInt();
            m_udpHandler->start(ip, port);
        });
        break;
    case AppMode::VU_REMOTE_ALMAZ:
        QMetaObject::invokeMethod(m_tcpHandler, [this]() {
            m_tcpHandler->connectToHost(
                appSettings().value("vu_settings_almaz/tcp_host_ip").toString(),
                appSettings().value("vu_settings_almaz/tcp_host_port").toInt());
        });
        QMetaObject::invokeMethod(m_udpHandler, [this]() {
            QString ip = appSettings()
                                .value("vu_settings_almaz/udp_ip", "0.0.0.0")
                                                                .toString();
            int port = appSettings()
                                .value("vu_settings_almaz/udp_port").toInt();
            m_udpHandler->start(ip, port);
        });
        break;
    }

    QMetaObject::invokeMethod(m_tcpHandler, [this]() {
        m_tcpHandler->startTimers();
    });

    LOG_INIT(CLASS_NAME, StatusInit::LOADED);
#ifdef _WIN32
    // m_ethTimer.start(1);
#endif
}

void NetworkManager::stop() {
    // Остановка таймера
    m_ethTimer.stop();
    QMetaObject::invokeMethod(m_tcpHandler, &TcpHandler::disconnect);
    QMetaObject::invokeMethod(m_ethHandler, &EthHandler::disconnectAdapter);
    LOG_INTERNAL(CLASS_NAME, "NetworkManager. останов.", "");
}

template<typename T>
T clamp(T value, T min_val, T max_val) {
    return (value < min_val) ? min_val : (value > max_val) ? max_val : value;
}

void NetworkManager::onTcpReceived(const TcpHeader& msg) {
#ifdef QT_DEBUG
    qDebug() << "TCP Message:" << msg.checkSum;
#endif
}

void NetworkManager::onUdpReceived(const QByteArray& msg) {
#ifdef QT_DEBUG
    qDebug() << Qt::endl << "UDP Message:" << msg.toHex(' ').toUpper();
#endif
}

void NetworkManager::onUdpReceivedDebug(const Message& msg) {
#ifdef QT_DEBUG
    qDebug() << "UDP Message debug:" << msg.payload;
#endif
}

void NetworkManager::onEthReceived(const QByteArray& msg) {
    // All ETH parsing and business logic is now handled in EthHandler::processIncomingFrame.
    Q_UNUSED(msg);
}
