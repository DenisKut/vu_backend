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
#ifdef _WIN32
    connect(&m_ethTimer, &QTimer::timeout, &m_ethHandler, &EthHandler::checkReceived);
    connect(&m_ethHandler, &EthHandler::ethTimerReceiveRestart, [this](){ m_ethTimer.start(1); });
#endif
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

#define TO_HEX(T,len) (QByteArray::fromRawData(reinterpret_cast<const char*>(T), len).toHex(' '))
#define TO_HEX_STR(T,len) (QString::fromLatin1(QByteArray::fromRawData(reinterpret_cast<const char*>(T), len).toHex(' ').toUpper()))

void NetworkManager::onEthReceived(const QByteArray& msg) {
    // Отладочный вывод тела Ethernet пакета целиком
    qDebug() << Qt::endl << "ETH Message:" << msg.toHex(' ').toUpper();
    // Парсинг входящих сообщений
    auto parsed = EthMessageFactory::parse(msg);
    if (parsed)
    {
        // Отладочный вывод номера принятого сообщения
        qDebug() << "parsed->mesNumber =" << TO_HEX_STR(&parsed->mesNumber, 2);

        // Проверяем приход сообщений:
        //-----------------------------------------------------------------------------------
        // Сообщение синхронизации БС (2.5)
        if(parsed->mesNumber == to_underlying_eth(ETH_MessageNumbers::BS_SYNC)) {
            // Получение указателя на сообщение
            auto msgBS_SYNC = reinterpret_cast<EthMessage_BS_SYNC*>(parsed.get());
            // Если удалось получить указатель
            if (msgBS_SYNC)
            {
                // Сохранение времени нового
                // периода синхронизации БС
                TimeProvider::instance().setBsPeriodNumber(msgBS_SYNC->periodNumber);
                TimeProvider::instance().setBsPeriodTime(msgBS_SYNC->periodTime);
                TimeProvider::instance().setBsPeriodWidth(msgBS_SYNC->periodWidth);

                // Отладочный вывод параметров сообщения 2.5
                qDebug() << "2.5 RECEIVED";
                qDebug() << "--------------------------------------------";
                qDebug() << "timeReceive =" << TO_HEX_STR(msgBS_SYNC->timeReceive, 5);              // Время приёма
                qDebug() << "timeBeginTrans =" << TO_HEX_STR(msgBS_SYNC->timeBeginTrans, 5);        // Время начала передачи
                qDebug() << "periodNumber =" << TO_HEX_STR(&msgBS_SYNC->periodNumber, 4);           // Порядковый номер нового периода
                qDebug() << "periodTime =" << TO_HEX_STR(msgBS_SYNC->periodTime, 5);               // Время начала нового периода
                qDebug() << "periodWidth =" << TO_HEX_STR(&msgBS_SYNC->periodWidth, 4);             // Значение периода синхронизации
                qDebug() << "--------------------------------------------";
            }
        }
        // Информация ориентации ППУ (11.1)
        else if(parsed->mesNumber == to_underlying_eth(ETH_MessageNumbers::PPU_POSITION)) {
            // Получение указателя на сообщение
            auto msgPPU_POSITION = reinterpret_cast<EthMessage_PPU_POSITION*>(parsed.get());
            // Если удалось получить указатель
            if (msgPPU_POSITION)
            {
                // Отладочный вывод параметров сообщения 11.1
                qDebug() << "11.1 RECEIVED";
                qDebug() << "--------------------------------------------";
                qDebug() << "timeReceive =" << TO_HEX_STR(msgPPU_POSITION->timeReceive, 5);         // Время приёма
                qDebug() << "timeBeginTrans =" << TO_HEX_STR(msgPPU_POSITION->timeBeginTrans, 5);   // Время начала передачи
                qDebug() << "lastCommand =" << TO_HEX_STR(&msgPPU_POSITION->lastCommand, 1);        // Код последней полученной команды
                qDebug() << "state =" << TO_HEX_STR(&msgPPU_POSITION->state, 1);                    // Состояние
                qDebug() << "ispr =" << TO_HEX_STR(&msgPPU_POSITION->ispr, 1);                      // Исправность оборудования
                qDebug() << "az =" << TO_HEX_STR(msgPPU_POSITION->az, 3);                           // Текущая координата по азимуту, град
                qDebug() << "azDelay =" << TO_HEX_STR(&msgPPU_POSITION->azDelay, 1);                // Задержка измерения координаты по азимуту, мс
                qDebug() << "an =" << TO_HEX_STR(msgPPU_POSITION->an, 3);                           // Текущая координата по углу места, град
                qDebug() << "anDelay =" << TO_HEX_STR(&msgPPU_POSITION->anDelay, 1);                // Задержка измерения координаты по углу места, мс
                qDebug() << "borders =" << TO_HEX_STR(&msgPPU_POSITION->borders, 1);                // Состояние границ
                qDebug() << "waitCommands =" << TO_HEX_STR(&msgPPU_POSITION->waitCommands, 1);      // Время ожидания команд, мс
                qDebug() << "waitTarget =" << TO_HEX_STR(&msgPPU_POSITION->waitTarget, 1);          // Время ожидания целеуказаний, мс
                qDebug() << "--------------------------------------------";

                // Отладочное чтение параметров времени - проверка
                // на то, что значения записались корректно:
                qDebug() << "getBsPeriodNumber =" << TimeProvider::instance().getBsPeriodNumber();
                qDebug() << "getBsPeriodTime =" << TimeProvider::instance().getBsPeriodTime();
                qDebug() << "getBsPeriodWidth =" << TimeProvider::instance().getBsPeriodWidth();
            }
        }
        //-----------------------------------------------------------------------------------
    }
}
