#include "NetworkManager.h"

NetworkManager::NetworkManager(TimeProvider* timeProvider, QObject* parent)
    : QObject(parent),
      m_timeProvider(timeProvider),
      m_tcpThread(new QThread(this)),  // Потоки создаются первыми
      m_udpThread(new QThread(this)),
      m_ethThread(new QThread(this))
{
    // Создаем обработчики БЕЗ РОДИТЕЛЯ
    m_tcpHandler = new TcpHandler(m_timeProvider);
    m_udpHandler = new UdpHandler(m_timeProvider);
    m_ethHandler = new EthHandler(m_timeProvider);

    // Перемещаем объекты в потоки перед инициализацией
    m_tcpHandler->moveToThread(m_tcpThread);
    m_udpHandler->moveToThread(m_udpThread);
    m_ethHandler->moveToThread(m_ethThread);

    QMetaObject::invokeMethod(m_tcpHandler, "initialize", Qt::QueuedConnection);
//    QMetaObject::invokeMethod(m_udpHandler, "initialize", Qt::QueuedConnection);
//    QMetaObject::invokeMethod(m_ethHandler, "initialize", Qt::QueuedConnection);

    // Запускаем потоки
    m_tcpThread->start();
    m_udpThread->start();
    m_ethThread->start();

    if(m_tcpThread->isRunning()) {
        m_tcpThread->setPriority(QThread::HighPriority);
    }
    if(m_ethThread->isRunning()) {
        m_ethThread->setPriority(QThread::TimeCriticalPriority);
    }

    // Инициализация через очередь событий
    QMetaObject::invokeMethod(m_tcpHandler, [this]() {
       // Подключение сигналов после перемещения в поток
       connect(m_tcpHandler, &TcpHandler::messageReceived,
              this, &NetworkManager::onTcpReceived, Qt::QueuedConnection);
    });

    QMetaObject::invokeMethod(m_ethHandler, [this]() {
#ifdef _WIN32
    connect(&m_ethTimer, &QTimer::timeout, &m_ethHandler, &EthHandler::checkReceived);
    connect(&m_ethHandler, &EthHandler::ethTimerReceiveRestart, [this](){ m_ethTimer.start(1); });
#else
    connect(m_ethHandler, &EthHandler::messageReceived,
            this, &NetworkManager::onEthReceived, Qt::QueuedConnection);
#endif
    });

//    connect(&m_udpHandler, &UdpHandler::datagramReceived, this, &NetworkManager::onUdpReceived);
//    connect(&m_udpHandler, &UdpHandler::datagramReceivedDebug, this, &NetworkManager::onUdpReceivedDebug);

}

NetworkManager::~NetworkManager() {
    stop();
    // Остановка потоков
    m_tcpThread->quit();
    m_udpThread->quit();
    m_ethThread->quit();

    // Ожидание завершения
    m_tcpThread->wait(1000);
    m_udpThread->wait(1000);
    m_ethThread->wait(1000);

    // Принудительное удаление при необходимости
    if(!m_tcpThread->isFinished()) {
        delete m_tcpThread;
    }
    if(!m_udpThread->isFinished()) {
        delete m_udpThread;
    }
    if(!m_ethThread->isFinished()) {
        delete m_ethThread;
    }
}

void NetworkManager::start() {
    QMetaObject::invokeMethod(m_tcpHandler, "connectToHost",
        Q_ARG(QString, to_qstring(AppConfig::instance().TCP_HOST_IP())),
        Q_ARG(quint16, AppConfig::instance().TCP_HOST_PORT()));
    QMetaObject::invokeMethod(m_tcpHandler, "startTimers");

    QMetaObject::invokeMethod(m_udpHandler, "startListening",
        Q_ARG(quint16, AppConfig::instance().UDP_PORT()));

    QMetaObject::invokeMethod(m_ethHandler, "start");
    m_ethTimer.start(1);
}


void NetworkManager::stop() {
    m_ethTimer.stop();
    QMetaObject::invokeMethod(m_tcpHandler, "disconnect");
    QMetaObject::invokeMethod(m_udpHandler, "stopListening");
    QMetaObject::invokeMethod(m_ethHandler, "disconnectAdapter");
}

// ППУ:
//----------------------------------------------------------
void NetworkManager::calculateCoefficientsPPU(double angleTarget,  double velocityTarget,
                              double angleCurrent, double* angleLast,
                              double* accelerationLast, double accelerationMax,
                              double angleMin, double velocityMin,
                              double KP, double KD, double dtLast, double dt,
                              double* C0, double* C1, double* C2) {


    double xDist = angleTarget - angleCurrent;
    if (xDist > 180.0) {
        xDist -= 360.0;
    } else if (xDist < -180.0) {
        xDist += 360.0;
    }
    double dx = angleCurrent - (*angleLast);
    if (dx > 180.0) {
        dx -= 360.0;
    } else if (dx < -180.0) {
        dx += 360.0;
    }
    double v1 = dx / dt;// + 0.5 * (*accelerationLast) * dt;
    double dv = velocityTarget - v1;
    double a1 = 0.0;

//        double breaking_dist = dv * dv / (2.0 * accelerationMax) + angleMin;

//        if (abs(xDist) > angleMin) {
//            // Максимальное ускорение в направлении цели
//            a1 = (xDist > 0) ? accelerationMax : -accelerationMax;

//            // Если расстояние торможения достигнуто, то тормозим:
//            if (abs(xDist) < breaking_dist) {
//                a1 = (dv < 0) ? -accelerationMax : accelerationMax;
//            }

//            // Плавное изменение ускорения:
//            if (abs(dv) < velocityMin) {
//                a1 = (*accelerationLast) + 0.5 * (a1 - (*accelerationLast));
//            }
//        } else {
        // ПД-регулятор
        a1 = KP * xDist + KD * dv;
//        }

    // Ограничение ускорения
    a1 = fmax(fmin(a1, accelerationMax), -accelerationMax);

//        // Плавное изменение ускорения:
//        if (abs(dv) < velocityMin) {
//            a1 = (*accelerationLast) + 0.8 * (a1 - (*accelerationLast));
//        }

    // Запоминаем предыдущие значения положения и ускорения точки 1
    *angleLast = angleCurrent;
    *accelerationLast = a1;

    // Установка коэффициентов
    *C0 = angleCurrent; // Начальное смещение (не используется)
    *C1 = v1;           // Текущая скорость (начальная для интервала)
    *C2 = 0.5 * a1;     // Ускорение, деленное на 2
}
//----------------------------------------------------------

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

void NetworkManager::onUdpReceivedDebug(const Message& msg) {
    qDebug() << "UDP Message debug:" << msg.payload;

//    // Задать ЦУ для ППУ
//    if (msg.payload.size() < 16) return;
//    QDataStream stream(msg.payload);
//    stream.setByteOrder(QDataStream::LittleEndian); // Порядок байтов должен совпадать с записью
//    double azTarget, anTarget;
//    stream >> azTarget >> anTarget;
//    AppConfig::instance().setModulePPU_CU(
//                AppConfig::instance().ModulePPU_num_RLMN_1,
//                azTarget,
//                anTarget);
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
                TimeProvider::instance().syncTimeBS();

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
                // Сохранить время БС, соответствующее сообщению 11.1:
                //-------------------------------------------------------------
                int64_t timeTemp;
                for (int i = 4; i >= 0; --i) { // Обратный порядок (Little-endian)
                    timeTemp = (timeTemp << 8) | msgPPU_POSITION->timeBeginTrans[i];
                }
                timeTemp &= 0x000000FFFFFFFFFF;
                TimeProvider::instance().setBsTimePPU(timeTemp);
                //-------------------------------------------------------------

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
                    static int mode = 0;
                    //if (mode == 0)
                    {
                        //mode = 1;
                        auto msg = EthMessageFactory::create<EthMessage_PPU_MOVE>();
                        if (mode == 0)
                        {
                            static bool isChangedImitPeriod = false;
                            if (!isChangedImitPeriod) {
                                // Пакет формата 4.6 для установки периода
                                // выдачи сообщений с имитатора 0.1 секунды:
                                const u_char buf[60] = {0x00, 0x0A, 0x35, 0x00,
                                                        0x23, 0x03, 0x81, 0x82,
                                                        0x83, 0x84, 0x85, 0x86,
                                                        0x20, 0x20, 0x00, 0x23,
                                                        0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x80, 0x04,
                                                        0x06, 0x14, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x14, 0x00,
                                                        0x14, 0x00, 0x28, 0x00,
                                                        0x19, 0x00, 0x05, 0x00,
                                                        0x64, 0x00, 0x1E, 0x00,
                                                        0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00
                                                       };
                                QByteArray data = QByteArray(
                                            reinterpret_cast<const char*>(buf),
                                            sizeof(buf)
                                            );
                                qDebug() << "Sending message to change period "
                                            "of messages on imitator:";
                                qDebug() << "ETH Message from VU to BS:"
                                         << Qt::endl
                                         << data.toHex(' ').toUpper();
                                isChangedImitPeriod = m_ethHandler->sendMessage(data);
                                if (isChangedImitPeriod) {
                                    mode = 1;
                                }
                            }

//                            uint32_t value = 0;
//                            msg.azC0[0] =  value & 0xFF;
//                            msg.azC0[1] = (value >> 8) & 0xFF;
//                            msg.azC0[2] = (value >> 16) & 0xFF;
//                            msg.azC1 = 0;
//                            msg.azC2 = 0;
//                            value = 0;
//                            msg.anC0[0] =  value & 0xFF;
//                            msg.anC0[1] = (value >> 8) & 0xFF;
//                            msg.anC0[2] = (value >> 16) & 0xFF;
//                            msg.anC1 = 0;
//                            msg.anC2 = 0;
//                            msg.azSector = 0;
//                            msg.anSector = 0;

//                            mode = 3;
//                            double value = 50;
//                            to_anglePPU(value, msg.azC0);
//                            msg.azC1 = 0;
//                            msg.azC2 = 0;
//                            to_anglePPU(value, msg.anC0);
//                            msg.anC1 = 0;
//                            msg.anC2 = 0;
//                            msg.azSector = 0;
//                            msg.anSector = 0;
////                            uint32_t value = 0x00332211;
////                            msg.azC0[0] =  value & 0xFF;
////                            msg.azC0[1] = (value >> 8) & 0xFF;
////                            msg.azC0[2] = (value >> 16) & 0xFF;
////                            msg.azC1 = 0x5544;
////                            msg.azC2 = 0x7766;
////                            value = 0x00AA9988;
////                            msg.anC0[0] =  value & 0xFF;
////                            msg.anC0[1] = (value >> 8) & 0xFF;
////                            msg.anC0[2] = (value >> 16) & 0xFF;
////                            msg.anC1 = 0xCCBB;
////                            msg.anC2 = 0xEEDD;
////                            msg.azSector = 0x3412;
////                            msg.anSector = 0x7856;
                        }
                        else if (mode == 1)
                        {
                            mode = 2;
//                            uint32_t value = 0;//10000;
//                            msg.azC0[0] =  value & 0xFF;
//                            msg.azC0[1] = (value >> 8) & 0xFF;
//                            msg.azC0[2] = (value >> 16) & 0xFF;
//                            msg.azC1 = 0;
//                            msg.azC2 = 0;
//                            value = 0;//10000;
//                            msg.anC0[0] =  value & 0xFF;
//                            msg.anC0[1] = (value >> 8) & 0xFF;
//                            msg.anC0[2] = (value >> 16) & 0xFF;
//                            msg.anC1 = 0;
//                            msg.anC2 = 0;
//                            msg.azSector = 0;
//                            msg.anSector = 0;
                        }
                        else if (mode == 2)
                        {
                            mode = 3;
//                            uint32_t value = 0;
//                            msg.azC0[0] =  value & 0xFF;
//                            msg.azC0[1] = (value >> 8) & 0xFF;
//                            msg.azC0[2] = (value >> 16) & 0xFF;
//                            msg.azC1 = 0;
//                            msg.azC2 = 0;
//                            value = 0;
//                            msg.anC0[0] =  value & 0xFF;
//                            msg.anC0[1] = (value >> 8) & 0xFF;
//                            msg.anC0[2] = (value >> 16) & 0xFF;
//                            msg.anC1 = 0;
//                            msg.anC2 = 0;
//                            msg.azSector = 0;
//                            msg.anSector = 0;
                        }
                        else if (mode == 3)
                        {
                            mode = 4;
                            double dt = 0.0000001*static_cast<double>(TimeProvider::instance().getBsPeriodWidth());
                            double dtLast = 0.0000001*static_cast<double>(TimeProvider::instance().getBsDeltaPPU());

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
//                            double aMax = 1.5; // 30.0 для dt = 0.1
//                            double KP = 0.5;
//                            double KD = 1.5;
                            double aMax = 30.0;
                            double KP = 3.0;
                            double KD = 6.0;

                            dt *= 3.0;

                            double azTarget = AppConfig::instance().getModulePPU_azTarget(
                                        AppConfig::instance().ModulePPU_num_RLMN_1);
                            double anTarget = AppConfig::instance().getModulePPU_anTarget(
                                        AppConfig::instance().ModulePPU_num_RLMN_1);

                            double azTargetVel = AppConfig::instance().getModulePPU_azTargetVel(
                                        AppConfig::instance().ModulePPU_num_RLMN_1);
                            double anTargetVel = AppConfig::instance().getModulePPU_anTargetVel(
                                        AppConfig::instance().ModulePPU_num_RLMN_1);

                            calculateCoefficientsPPU(azTarget, azTargetVel,
                                                     az, &azLast, &azAccLast,
                                                     aMax, 10.0, 10.0, KP, KD, dtLast, dt,
                                                     &azC0, &azC1, &azC2);

                            calculateCoefficientsPPU(anTarget, anTargetVel,
                                                     an, &anLast, &anAccLast,
                                                     aMax, 10.0, 10.0, KP, KD, dtLast, dt,
                                                     &anC0, &anC1, &anC2);

                            azC2 *= 2.0;
                            anC2 *= 2.0;

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

                            // Отправка отладочных данных на форму для отображения:
                            //-------------------------------------------------------------
//                            QByteArray dataDebug;
//                            QString str;

//                            str  = QString("az = %1\n"
//                                           "an = %2\n"
//                                           "\n"
//                                           "C0 = %3\n"
//                                           "C1 = %4\n"
//                                           "C2 = %5\n")
//                                    .arg(AppConfig::instance().getModulePPU_az(
//                                             AppConfig::instance().ModulePPU_num_RLMN_1))
//                                    .arg(AppConfig::instance().getModulePPU_an(
//                                             AppConfig::instance().ModulePPU_num_RLMN_1))
//                                    .arg(azC0)
//                                    .arg(azC1)
//                                    .arg(azC2);
//                            dataDebug = str.toUtf8();

                            QByteArray dataDebug;
                            QDataStream stream(&dataDebug, QIODevice::WriteOnly);
                            stream << AppConfig::instance().getModulePPU_az(
                                          AppConfig::instance().ModulePPU_num_RLMN_1)
                                   << AppConfig::instance().getModulePPU_an(
                                          AppConfig::instance().ModulePPU_num_RLMN_1)
                                   << azC0 << azC1 << azC2
                                   << anC0 << anC1 << anC2
                                   << AppConfig::instance().getModulePPU_azTarget(
                                          AppConfig::instance().ModulePPU_num_RLMN_1)
                                   << AppConfig::instance().getModulePPU_anTarget(
                                          AppConfig::instance().ModulePPU_num_RLMN_1)
                                   << AppConfig::instance().getModulePPU_azTargetVel(
                                          AppConfig::instance().ModulePPU_num_RLMN_1)
                                   << AppConfig::instance().getModulePPU_anTargetVel(
                                          AppConfig::instance().ModulePPU_num_RLMN_1)
                                   << AppConfig::instance().getModulePPU_isRemote(
                                          AppConfig::instance().ModulePPU_num_RLMN_1);

//                            m_udpHandler.sendDatagramDebug(QHostAddress::LocalHost, 12346, dataDebug);
                            m_tcpHandler->sendMessageRMO(dataDebug);
                            //-------------------------------------------------------------

                            QByteArray data = msg.serializeFull();
                            qDebug() << "Sending message to move the PPU:";
                            qDebug() << "Time BS:" << TimeProvider::instance().getTimeForRequestBS();
                            qDebug() << "ETH Message from VU to BS:" << Qt::endl << data.toHex(' ').toUpper();
                            m_ethHandler->sendMessage(data);
                        }
                        else if (mode == 4) {
                            mode = 5;
                        }
                        else if (mode == 5) {
                            mode = 3;
                        }
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
                    m_ethHandler->sendMessage(data);
                }
            }
        }
        //-----------------------------------------------------------------------------------
    }
}
