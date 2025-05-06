#include "NetworkManager.h"

/**
 * @brief макрос, управляющий назначением соединения
 * @param VU_LOCALHOST - voimit
 * @else @param VU_REMOTE_ALMAZ - almaz stend
 */
#define VU_LOCALHOST

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
#ifdef VU_LOCALHOST
    m_tcpHandler.connectToHost(
        appSettings().value("vu_settings/tcp_host_ip").toString(),
        appSettings().value("vu_settings/tcp_host_port").toInt());
    m_udpHandler.startListening(appSettings().value("vu_settings/udp_port").toInt());
#endif
#ifdef VU_REMOTE_ALMAZ
    m_tcpHandler.connectToHost(
        appSettings().value("vu_settings_almaz/tcp_host_ip").toString(),
        appSettings().value("vu_settings_almaz/tcp_host_port").toInt());
    m_udpHandler.startListening(appSettings().value("vu_settings_almaz/udp_port").toInt());
#endif
    m_ethHandler.start();
    m_ethTimer.start(1);
    LOG_INIT(typeid(this).name(), StatusInit::LOADED);
}

void NetworkManager::stop() {
    m_ethTimer.stop();
    m_tcpHandler.disconnect();
    m_udpHandler.stopListening();
    m_ethHandler.disconnectAdapter();
    LOG_INTERNAL(typeid(this).name(), "NetworkManager. останов.", "");
}



template<typename T>
T clamp(T value, T min_val, T max_val) {
    return (value < min_val) ? min_val : (value > max_val) ? max_val : value;
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
//    // Отладочный вывод тела Ethernet пакета целиком
//    qDebug() << Qt::endl << "ETH Message:" << msg.toHex(' ').toUpper();
    // Парсинг входящих сообщений
    auto parsed = EthMessageFactory::parse(msg);
    if (parsed)
    {
        // Отладочный вывод тела Ethernet пакета целиком
        qDebug() << Qt::endl << "ETH Message:" << msg.toHex(' ').toUpper();

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

                LOG(ProtocolMessageLog(typeid(this).name(), LogLevel::MessageReceived,Protocol::RAW_MAC, msg, 
                Logger::jobj_toString(msgBS_SYNC->to_json())));
            }
        }
        // Информация ориентации ППУ (11.1)
        else if(parsed->mesNumber == to_underlying_eth(ETH_MessageNumbers::PPU_POSITION)) {
            // Получение указателя на сообщение
            auto msgPPU_POSITION = reinterpret_cast<EthMessage_PPU_POSITION*>(parsed.get());
            // Если удалось получить указатель
            if (msgPPU_POSITION)
            {
                // Заполнение структуры ППУ:
                //-------------------------------------------------------------
                // Код последней полученной команды
                AppConfig::instance().setModulePPU_lastCommand(
                            AppConfig::instance().ModulePPU_num_RLMN_1,
                            msgPPU_POSITION->lastCommand);
                // Состояние
                AppConfig::instance().setModulePPU_state(
                            AppConfig::instance().ModulePPU_num_RLMN_1,
                            msgPPU_POSITION->state);
                // Исправность оборудования
                AppConfig::instance().setModulePPU_ispr(
                            AppConfig::instance().ModulePPU_num_RLMN_1,
                            msgPPU_POSITION->ispr);
                // Текущая координата по азимуту, град
                AppConfig::instance().setModulePPU_az(
                            AppConfig::instance().ModulePPU_num_RLMN_1,
                            msgPPU_POSITION->az);
                // Задержка измерения координаты по азимуту, мс
                AppConfig::instance().setModulePPU_azDelay(
                            AppConfig::instance().ModulePPU_num_RLMN_1,
                            msgPPU_POSITION->azDelay);
                // Текущая координата по углу места, град
                AppConfig::instance().setModulePPU_an(
                            AppConfig::instance().ModulePPU_num_RLMN_1,
                            msgPPU_POSITION->an);
                // Задержка измерения координаты по углу места, мс
                AppConfig::instance().setModulePPU_anDelay(
                            AppConfig::instance().ModulePPU_num_RLMN_1,
                            msgPPU_POSITION->anDelay);
                // Состояние границ
                AppConfig::instance().setModulePPU_borders(
                            AppConfig::instance().ModulePPU_num_RLMN_1,
                            msgPPU_POSITION->borders);
                // Время ожидания команд, мс
                AppConfig::instance().setModulePPU_waitCommands(
                            AppConfig::instance().ModulePPU_num_RLMN_1,
                            msgPPU_POSITION->waitCommands);
                // Время ожидания целеуказаний, мс
                AppConfig::instance().setModulePPU_waitTarget(
                            AppConfig::instance().ModulePPU_num_RLMN_1,
                            msgPPU_POSITION->waitTarget);
                //-------------------------------------------------------------

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
                LOG(ProtocolMessageLog(typeid(this).name(), LogLevel::MessageReceived, Protocol::RAW_MAC,
                    msg, Logger::jobj_toString(msgPPU_POSITION->to_json())));
                // Отладочный вывод параметров ППУ
                qDebug() << "PPU";
                qDebug() << "--------------------------------------------";
                qDebug() << "lastCommand =" <<
                            AppConfig::instance().getModulePPU_lastCommand(
                                AppConfig::instance().ModulePPU_num_RLMN_1); // Код последней полученной команды
                qDebug() << "state =" <<
                            AppConfig::instance().getModulePPU_state(
                                AppConfig::instance().ModulePPU_num_RLMN_1); // Состояние
                qDebug() << "ispr =" <<
                            AppConfig::instance().getModulePPU_ispr(
                                AppConfig::instance().ModulePPU_num_RLMN_1); // Исправность оборудования
                qDebug() << "az =" <<
                            AppConfig::instance().getModulePPU_az(
                                AppConfig::instance().ModulePPU_num_RLMN_1); // Текущая координата по азимуту, град
                qDebug() << "azDelay =" <<
                            AppConfig::instance().getModulePPU_azDelay(
                                AppConfig::instance().ModulePPU_num_RLMN_1); // Задержка измерения координаты по азимуту, мс
                qDebug() << "an =" <<
                            AppConfig::instance().getModulePPU_an(
                                AppConfig::instance().ModulePPU_num_RLMN_1); // Текущая координата по углу места, град
                qDebug() << "anDelay =" <<
                            AppConfig::instance().getModulePPU_anDelay(
                                AppConfig::instance().ModulePPU_num_RLMN_1); // Задержка измерения координаты по углу места, мс
                qDebug() << "borders =" <<
                            AppConfig::instance().getModulePPU_borders(
                                AppConfig::instance().ModulePPU_num_RLMN_1); // Состояние границ
                qDebug() << "waitCommands =" <<
                            AppConfig::instance().getModulePPU_waitCommands(
                                AppConfig::instance().ModulePPU_num_RLMN_1); // Время ожидания команд, мс
                qDebug() << "waitTarget =" <<
                            AppConfig::instance().getModulePPU_waitTarget(
                                AppConfig::instance().ModulePPU_num_RLMN_1); // Время ожидания целеуказаний, мс
                qDebug() << "--------------------------------------------";

                // Отладочное чтение параметров времени - проверка
                // на то, что значения записались корректно:
                qDebug() << "getBsPeriodNumber =" << TimeProvider::instance().getBsPeriodNumber();
                qDebug() << "getBsPeriodTime =" << TimeProvider::instance().getBsPeriodTime();
                qDebug() << "getBsPeriodWidth =" << TimeProvider::instance().getBsPeriodWidth();

                // Если ППУ выключено, то отправить сообщение о включении:
                uint8_t state = AppConfig::instance().getModulePPU_state(
                    AppConfig::instance().ModulePPU_num_RLMN_1); // Состояние ППУ
                if (state & AppConfig::instance().ModulePPU_state_TurnedOn)
                {
                    qDebug() << "PPU is turned On" << Qt::endl;

                    // Если ППУ включено, то подвинуть его в координаты 10, 10:
                    static int mode = 3;
                    //if (mode == 0)
                    {
                        //mode = 1;
                        auto msg = EthMessageFactory::create<EthMessage_PPU_MOVE>();
                        if (mode == 0)
                        {
                            mode = 3;
                            double value = 50;
                            to_anglePPU(value, msg.azC0);
                            msg.azC1 = 0;
                            msg.azC2 = 0;
                            to_anglePPU(value, msg.anC0);
                            msg.anC1 = 0;
                            msg.anC2 = 0;
                            msg.azSector = 0;
                            msg.anSector = 0;
//                            uint32_t value = 0x00332211;
//                            msg.azC0[0] =  value & 0xFF;
//                            msg.azC0[1] = (value >> 8) & 0xFF;
//                            msg.azC0[2] = (value >> 16) & 0xFF;
//                            msg.azC1 = 0x5544;
//                            msg.azC2 = 0x7766;
//                            value = 0x00AA9988;
//                            msg.anC0[0] =  value & 0xFF;
//                            msg.anC0[1] = (value >> 8) & 0xFF;
//                            msg.anC0[2] = (value >> 16) & 0xFF;
//                            msg.anC1 = 0xCCBB;
//                            msg.anC2 = 0xEEDD;
//                            msg.azSector = 0x3412;
//                            msg.anSector = 0x7856;
                        }
                        else if (mode == 1)
                        {
                            mode = 2;
                            uint32_t value = 10000;
                            msg.azC0[0] =  value & 0xFF;
                            msg.azC0[1] = (value >> 8) & 0xFF;
                            msg.azC0[2] = (value >> 16) & 0xFF;
                            msg.azC1 = 0;
                            msg.azC2 = 0;
                            value = 10000;
                            msg.anC0[0] =  value & 0xFF;
                            msg.anC0[1] = (value >> 8) & 0xFF;
                            msg.anC0[2] = (value >> 16) & 0xFF;
                            msg.anC1 = 0;
                            msg.anC2 = 0;
                            msg.azSector = 0;
                            msg.anSector = 0;
                        }
                        else if (mode == 2)
                        {
                            //mode = 3;
                            uint32_t value = 0;
                            msg.azC0[0] =  value & 0xFF;
                            msg.azC0[1] = (value >> 8) & 0xFF;
                            msg.azC0[2] = (value >> 16) & 0xFF;
                            msg.azC1 = 0;
                            msg.azC2 = 0;
                            value = 0;
                            msg.anC0[0] =  value & 0xFF;
                            msg.anC0[1] = (value >> 8) & 0xFF;
                            msg.anC0[2] = (value >> 16) & 0xFF;
                            msg.anC1 = 0;
                            msg.anC2 = 0;
                            msg.azSector = 0;
                            msg.anSector = 0;
                        }
                        else if (mode == 3)
                        {
                            double dt = 0.0000001*static_cast<double>(TimeProvider::instance().getBsPeriodWidth());

                            double az = AppConfig::instance().getModulePPU_az(
                                AppConfig::instance().ModulePPU_num_RLMN_1);
                            double an = AppConfig::instance().getModulePPU_an(
                                AppConfig::instance().ModulePPU_num_RLMN_1);

                            static double azLast = 0;
                            static double azAccLast = 0;
                            static double anLast = 0;
                            static double anAccLast = 0;
                            double azC0, anC0, azC1, anC1, azC2, anC2;
                            uint16_t azC1u, azC2u, anC1u, anC2u;
                            double aMax = 1.5; // 30.0 для dt = 0.1
                            double KP = 0.5;
                            double KD = 1.5;

                            calculateCoefficientsPPU(0, 0,
                                                     az, &azLast, &azAccLast,
                                                     aMax, 10.0, 10.0, KP, KD, dt,
                                                     &azC0, &azC1, &azC2);

                            calculateCoefficientsPPU(0, 0,
                                                     an, &anLast, &anAccLast,
                                                     aMax, 10.0, 10.0, KP, KD, dt,
                                                     &anC0, &anC1, &anC2);

                            azC1u = azC1 * 65535.0 / 360.0;
                            azC2u = azC2 * 65535.0 / 360.0;
                            anC1u = anC1 * 65535.0 / 360.0;
                            anC2u = anC2 * 65535.0 / 360.0;

                            qDebug() << "azCur =" << azC0;
                            qDebug() << "azVel =" << azC1 << "=" << azC1u;
                            qDebug() << "azAcc =" << 2.0 * azC2 << "=" << azC2u;
                            qDebug() << "anCur =" << anC0;
                            qDebug() << "anVel =" << anC1 << "=" << anC1u;
                            qDebug() << "anAcc =" << 2.0 * anC2 << "=" << anC2u;

//                            qDebug() << "azC0 =" << azC0;
//                            qDebug() << "azC1 =" << azC1 << "=" << azC1u;
//                            qDebug() << "azC2 =" << azC2 << "=" << azC2u;

//                            qDebug() << "anC0 =" << anC0;
//                            qDebug() << "anC1 =" << anC1 << "=" << anC1u;
//                            qDebug() << "anC2 =" << anC2 << "=" << anC2u;

                            msg.azC0[0] = 0;
                            msg.azC0[1] = 0;
                            msg.azC0[2] = 0;
                            msg.azC1 = azC1u;
                            msg.azC2 = azC2u;

                            msg.anC0[0] = 0;
                            msg.anC0[1] = 0;
                            msg.anC0[2] = 0;
                            msg.anC1 = anC1u;
                            msg.anC2 = anC2u;

                            msg.azSector = 0;
                            msg.anSector = 0;
                        }
                        QByteArray data = msg.serializeFull();
                        qDebug() << "Sending message to move the PPU:";
                        qDebug() << "ETH Message from VU to BS:" << Qt::endl << data.toHex(' ').toUpper();
                        m_ethHandler.sendMessage(data);
                        LOG(ProtocolMessageLog(typeid(this).name(), LogLevel::MessageSent, Protocol::RAW_MAC, 
                        data.toHex(' ').toUpper(), Logger::jobj_toString(msg.to_json())));
                    }
                }
                else
                {
                    qDebug() << "PPU is turned Off" << Qt::endl;
                    auto msg = EthMessageFactory::create<EthMessage_PPU_ENABLE>();
                    msg.state = 1; // Включить ППУ
                    QByteArray data = msg.serializeFull();
                    qDebug() << "Sending message to turn on the PPU:";
                    qDebug() << "ETH Message from VU to BS:" << Qt::endl << data.toHex(' ').toUpper();
                    m_ethHandler.sendMessage(data);
                    LOG(ProtocolMessageLog(typeid(this).name(), LogLevel::MessageSent, Protocol::RAW_MAC, 
                    data.toHex(' ').toUpper(), Logger::jobj_toString(msg.to_json())));
                }
            }
        }
        //-----------------------------------------------------------------------------------
    }
}
