#include "NetworkManager.h"
#include "utils/env_loader/Settings.h"
#include <QMetaType>
NetworkManager::NetworkManager(QObject* parent)
    : QObject(parent),
      m_tcpHandler(new TcpHandler()),
      m_udpHandler(new UdpHandler()),
      m_ethHandler(new EthHandler()),
      m_tcpThread(new QThread(this)),
      m_udpThread(new QThread(this)),
      m_ethThread(new QThread(this)) {

    // Установка приоритетов потоков
    m_ethThread->setPriority(QThread::TimeCriticalPriority);
    m_udpThread->setPriority(QThread::HighPriority);
    m_tcpThread->setPriority(QThread::NormalPriority);

    qRegisterMetaType<RmoMessage_TargetPPU>("RmoMessage_TargetPPU");

    // Перемещение обработчиков в потоки
    m_tcpHandler->moveToThread(m_tcpThread);
    m_udpHandler->moveToThread(m_udpThread);
    m_ethHandler->moveToThread(m_ethThread);

    // Установка колбэка для отправки в EthHandler
    m_tcpHandler->setEthSendCallback([this](const QByteArray& data) {
        this->sendEth(data);
    });
    // Установка колбэка для передачи сообщений из EthHandler в TcpHandler
    m_ethHandler->setTcpSendCallback([this](QSharedPointer<RmoBaseMessage> msg) {
        this->sendTcpRmo(msg);
    });

    // Подключение сигналов и слотов для потоков
    connect(m_tcpThread, &QThread::started, m_tcpHandler,
                            &TcpHandler::initialize);
    connect(m_ethThread, &QThread::started, m_ethHandler,
                            &EthHandler::initialize);

    // Подключение сигналов и слотов для обработчиков
    connect(m_tcpHandler, &TcpHandler::messageReceived, this,
                                    &NetworkManager::onTcpReceived,
                                    Qt::QueuedConnection);
    connect(m_udpHandler, &UdpHandler::packetReceived, this,
                                    &NetworkManager::onUdpReceived,
                                    Qt::QueuedConnection);
    connect(m_ethHandler, &EthHandler::messageReceived, this,
                                    &NetworkManager::onEthReceived,
                                    Qt::QueuedConnection);

#ifdef _WIN32
    // connect(&m_ethTimer, &QTimer::timeout, m_ethHandler,
	//                                            &EthHandler::checkReceived);
#endif
    // Запуск потоков
    m_tcpThread->start();
    m_udpThread->start();
    m_ethThread->start();
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
		qDebug()<<"VU_LOCALHOST selected";
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
        qDebug()<<"VU_REMOTE_ALMAZ selected";
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
    Q_UNUSED(msg);
#ifdef QT_DEBUG
    qDebug() << "TCP Message:" << msg.checkSum;
#endif
}

void NetworkManager::onUdpReceived(const QByteArray& msg) {
    Q_UNUSED(msg);
#ifdef QT_DEBUG
    qDebug() << Qt::endl << "UDP Message:" << msg.toHex(' ').toUpper();
#endif
}

void NetworkManager::onUdpReceivedDebug(const UdpMessage& msg) {
    Q_UNUSED(msg);
#ifdef QT_DEBUG
    qDebug() << "UDP Message debug:" << msg.payload;
#endif
}

void NetworkManager::onEthReceived(const QByteArray& msg) {
    // All ETH parsing and business logic is now handled in EthHandler::processIncomingFrame.
    Q_UNUSED(msg);
}

// --- Потокобезопасная отправка UDP ---
void NetworkManager::sendUdp(const QByteArray& data) {
    if (m_udpHandler) {
        QMetaObject::invokeMethod(m_udpHandler, [this, data]() {
            m_udpHandler->sendMessage(data);
        }, Qt::QueuedConnection);
    }
}

// --- Потокобезопасная отправка ETH ---
void NetworkManager::sendEth(const QByteArray& data) {
    if (m_ethHandler) {
        QMetaObject::invokeMethod(m_ethHandler, [this, data]() {
            m_ethHandler->sendMessage(data);
        }, Qt::QueuedConnection);
    }
}

// --- Потокобезопасная отправка TCP (для RMO) ---
void NetworkManager::sendTcpRmo(QSharedPointer<RmoBaseMessage> msg) {
    if (m_tcpHandler && msg) {
        QMetaObject::invokeMethod(m_tcpHandler, [this, msg]() {
            m_tcpHandler->sendRmoMessageToAll(msg);
        }, Qt::QueuedConnection);
    }
}
