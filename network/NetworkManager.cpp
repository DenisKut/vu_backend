#include "NetworkManager.h"

NetworkManager::NetworkManager(TimeProvider* timeProvider, QObject* parent)
    : QObject(parent),
      m_timeProvider(timeProvider),
      m_ethTimer(this),
      m_tcpHandler(timeProvider),
      m_udpHandler(timeProvider),
      m_ethHandler(timeProvider)
{
    connect(&m_tcpHandler, &TcpHandler::messageReceived, this, &NetworkManager::onTcpReceived);
    connect(&m_udpHandler, &UdpHandler::datagramReceived, this, &NetworkManager::onUdpReceived);
    connect(&m_ethHandler, &EthHandler::messageReceived, this, &NetworkManager::onEthReceived);
    connect(&m_ethTimer, &QTimer::timeout, &m_ethHandler, &EthHandler::checkReceived);
    connect(&m_ethHandler, &EthHandler::ethTimerReceiveRestart, [this](){ m_ethTimer.start(1); });
}

NetworkManager::~NetworkManager() {
    stop();
}

void NetworkManager::start() {
    m_tcpHandler.connectToHost(
        to_qstring(
            AppConfig::instance().TCP_HOST_IP()),
            AppConfig::instance().TCP_HOST_PORT()
        );
    m_udpHandler.startListening(AppConfig::instance().UDP_PORT());
    m_ethHandler.start();
    m_ethTimer.start(1);
}

void NetworkManager::stop() {
    m_ethTimer.stop();
    m_tcpHandler.disconnect();
    m_udpHandler.stopListening();
    m_ethHandler.disconnectAdapter();
}

void NetworkManager::onTcpReceived(const TcpHeader& msg) {
    qDebug() << "TCP Message:" << msg.checkSum;
}

void NetworkManager::onUdpReceived(const Message& msg) {
    qDebug() << "UDP Message:" << msg.payload;
}

void NetworkManager::onEthReceived(const QByteArray& msg) {
    qDebug() << "ETH Message:";// << msg;
    // Парсинг
    auto parsed = EthMessageFactory::parse(msg);

    // Проверяем приход сообщений:
    //-----------------------------------------------------------------------------------
    // Сообщение синхронизации БС (2.5)
    if(auto* msgBS_SYNC = reinterpret_cast<EthMessage_BS_SYNC*>(parsed.get())) {
        qDebug() << "2.5 RECEIVED";
        qDebug() << "--------------------------------------------";
        qDebug() << "timeReceive =" << msgBS_SYNC->timeReceive;         // Время приёма
        qDebug() << "timeBeginTrans =" << msgBS_SYNC->timeBeginTrans;   // Время начала передачи
        qDebug() << "periodNumber =" << msgBS_SYNC->periodNumber;       // Порядковый номер нового периода
        qDebug() << "periodTime =" << msgBS_SYNC->periodTime;           // Время начала нового периода
        qDebug() << "periodWidth =" << msgBS_SYNC->periodWidth;         // Значение периода синхронизации
        qDebug() << "--------------------------------------------";
    }
    // Информация ориентации ППУ (11.1)
    if(auto* msgPPU_POSITION = reinterpret_cast<EthMessage_PPU_POSITION*>(parsed.get())) {
        qDebug() << "11.1 RECEIVED";
        qDebug() << "--------------------------------------------";
        qDebug() << "timeReceive =" << msgPPU_POSITION->timeReceive;        // Время приёма
        qDebug() << "timeBeginTrans =" << msgPPU_POSITION->timeBeginTrans;  // Время начала передачи
        qDebug() << "lastCommand =" << msgPPU_POSITION->lastCommand;    // Код последней полученной команды
        qDebug() << "state =" << msgPPU_POSITION->state;                // Состояние
        qDebug() << "ispr =" << msgPPU_POSITION->ispr;                  // Исправность оборудования
        qDebug() << "az =" << msgPPU_POSITION->az;                      // Текущая координата по азимуту, град
        qDebug() << "azDelay =" << msgPPU_POSITION->azDelay;            // Задержка измерения координаты по азимуту, мс
        qDebug() << "an =" << msgPPU_POSITION->an;                      // Текущая координата по углу места, град
        qDebug() << "anDelay =" << msgPPU_POSITION->anDelay;            // Задержка измерения координаты по углу места, мс
        qDebug() << "borders =" << msgPPU_POSITION->borders;            // Состояние границ
        qDebug() << "waitCommands =" << msgPPU_POSITION->waitCommands;  // Время ожидания команд, мс
        qDebug() << "waitTarget =" << msgPPU_POSITION->waitTarget;      // Время ожидания целеуказаний, мс
        qDebug() << "--------------------------------------------";
    }
    //-----------------------------------------------------------------------------------
}
