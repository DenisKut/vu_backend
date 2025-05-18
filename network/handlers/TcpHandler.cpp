#include "TcpHandler.h"
#include <QHostAddress>
#include <QDebug>


TcpHandler::TcpHandler(QObject *parent)
    : QObject(parent)
{
    // ...инициализация в initialize()...
}

void TcpHandler::initialize()
{
    m_socket = new QTcpSocket(this);
    m_serverRMO = new QTcpServer(this);
    m_wsServer = new QWebSocketServer(QStringLiteral("RMO WebSocket Server"), QWebSocketServer::NonSecureMode, this);
    m_registrationTimer = new QTimer(this);
    m_periodicTimer = new QTimer(this);
    m_reconnectTimer = new QTimer(this);

    Q_ASSERT_X(m_socket, "TcpHandler", "Failed to create TCP socket");
    Q_ASSERT_X(m_serverRMO, "TcpHandler", "Failed to create TCP server");
    Q_ASSERT_X(m_wsServer, "TcpHandler", "Failed to create WebSocket server");

    m_socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    // --- TCP сервер ---
    if (!initServer()) {
        qFatal("Failed to initialize TCP server");
    }
    connect(m_serverRMO, &QTcpServer::newConnection, this,
                                &TcpHandler::incomingConnection,
                                Qt::QueuedConnection);

    // --- WebSocket сервер ---
    if (!initWebSocketServer()) {
        qWarning() << "WebSocket server not started";
    } else {
        connect(m_wsServer, &QWebSocketServer::newConnection, this,
                                &TcpHandler::onWebSocketNewConnection);
    }

    // --- Основное TCP-подключение ---
    m_mainSocketThread = new QThread(this);
    QTcpSocket* mainSocket = new QTcpSocket();
    m_mainConnection = new TcpConnectionHandler(mainSocket);
    mainSocket->moveToThread(m_mainSocketThread);
    m_mainConnection->moveToThread(m_mainSocketThread);
    connect(m_mainSocketThread, &QThread::finished, mainSocket,
                                                    &QObject::deleteLater);
    connect(m_mainSocketThread, &QThread::finished, m_mainConnection,
                                                    &QObject::deleteLater);
    connect(m_mainSocketThread, &QThread::finished, m_mainSocketThread,
                                                    &QObject::deleteLater);
    connect(m_mainConnection, &TcpConnectionHandler::finished,
                                                    m_mainSocketThread,
                                                    &QThread::quit);
    connect(m_mainConnection, &TcpConnectionHandler::messageReceived,
                                                    this, [this]
                                                    (const QByteArray& data){
        processIncomingMessage(data);
    });
    m_mainSocketThread->start();

    // --- Основной клиентский сокет ---
    connect(m_socket, &QTcpSocket::connected, this,
                                                &TcpHandler::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this,
                                                &TcpHandler::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this,
                                                &TcpHandler::onReadyRead);
    #if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    // Для Qt 5.15 и новее используем новый сигнал errorOccurred
    connect(m_socket, &QAbstractSocket::errorOccurred,
            this, &TcpHandler::onError);
#else
    // Для старых версий используем старый сигнал error с приведением типа
    connect(m_socket, static_cast<void (QAbstractSocket::*)
                                  (QAbstractSocket::SocketError)>
                                            (&QAbstractSocket::error),
            this, &TcpHandler::onError);
#endif


    m_reconnectTimer->setInterval(800);
    connect(m_reconnectTimer, &QTimer::timeout, this,
                                            &TcpHandler::reconnect);

    connect(m_registrationTimer, &QTimer::timeout, this,
                                            &TcpHandler::handleRegistration);
    connect(m_periodicTimer, &QTimer::timeout, this,
                                            &TcpHandler::sendPeriodicMessages);
}

TcpHandler::~TcpHandler() {
    disconnect();
    delete m_socket;
    delete m_serverRMO;
    delete m_registrationTimer;
    delete m_periodicTimer;
    delete m_reconnectTimer;
    delete m_wsServer;
    // Клиенты удаляются автоматически через deleteLater()
}

void TcpHandler::startTimers() {
    m_reconnectTimer->start(1000);
}

void TcpHandler::stopTimers() {
    m_registrationTimer->stop();
    m_periodicTimer->stop();
    m_reconnectTimer->stop();
}

bool TcpHandler::initServer() {
    constexpr quint16 startPort = 23458;
    constexpr quint16 maxAttempts = 10;
    for (quint16 port = startPort; port < startPort + maxAttempts; ++port) {
        if (m_serverRMO->listen(QHostAddress::Any, port)) {
            qInfo() << "TCP server started on port" << port;
            return true;
        }
    }
    qCritical() << "Failed to start TCP server after" << maxAttempts
                << "attempts";
    return false;
}

bool TcpHandler::initWebSocketServer() {
    constexpr quint16 wsPort = 23457;
    if (m_wsServer->listen(QHostAddress::Any, wsPort)) {
        qInfo() << "WebSocket server started on port" << wsPort;
        return true;
    }
    return false;
}

void TcpHandler::connectToHost(const QString& host, quint16 port) {
    m_host = host;
    m_port = port;
    qInfo() << "Connecting to" << host << ":" << port;
    m_socket->connectToHost(host, port, QIODevice::ReadWrite,
                                                QAbstractSocket::AnyIPProtocol);
}

void TcpHandler::disconnect() {
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
    }
    m_registrationTimer->stop();
    m_periodicTimer->stop();
}

void TcpHandler::onConnected() {
    std::cout << "Connected to TCP host" << std::endl;
    int timer = 800;
    m_reconnectTimer->stop();
    m_registrationTimer->start(timer); // Отправка 0x01
    LOG_INTERNAL(CLASS_NAME, "СОКЕТ TCP",
                    QString("ПОДКЛЮЧЁН, отправка раз в %1 мсек.").arg(timer));
}

void TcpHandler::onDisconnected() {

    std::cout << "Disconnected from TCP host" << std::endl;
    m_isRegistered = false;
    m_registrationTimer->stop();
    m_periodicTimer->stop();
    if (!m_reconnectTimer->isActive()) { // Защита от повторного запуска
        m_reconnectTimer->start();
    }
    LOG_INTERNAL(CLASS_NAME, "СОКЕТ TCP", "ОТКЛЮЧЁН");
}

void TcpHandler::reconnect() {
    if (!m_socket || m_socket->state() == QAbstractSocket::ConnectedState)
        return;

    // Сбрасываем соединение если не в состоянии Unconnected
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->abort();
    }

    qInfo() << "Reconnecting to" << m_host << ":" << m_port;
    m_socket->connectToHost(m_host, m_port);
}

void TcpHandler::onRmoDisconnected(TcpConnectionHandler* handler) {
    int idx = m_rmoConnections.indexOf(handler);
    if (idx != -1) {
        QTcpSocket* sock = handler->getSocket();
        if (sock) {
            qDebug() << "RMO TCP disconnected, descriptor:"
                     << sock->socketDescriptor()
                     << "total clients:" << m_rmoConnections.size() - 1;
        } else {
            qDebug() << "RMO TCP disconnected (unknown descriptor)";
        }
        m_rmoConnections.remove(idx);
        handler->deleteLater();
    }
}

void TcpHandler::incomingConnection() {
    // Этот слот вызывается в потоке m_rmoThread!
    while (m_serverRMO->hasPendingConnections()) {
        QTcpSocket* client = m_serverRMO->nextPendingConnection();
        if (client) {
            // ВАЖНО: не перемещаем сокет между потоками!
            // Создаём обработчик в этом же потоке:
            TcpConnectionHandler* handler = new TcpConnectionHandler(client,
                                                                    nullptr);

            connect(handler, &TcpConnectionHandler::finished, this, [this,
                                                                     handler](){
                onRmoDisconnected(handler);
            });

            connect(handler, &TcpConnectionHandler::messageReceived, this,
                [this](const QByteArray& data) {
                    processIncomingMessageRMO(data);
                });

            m_rmoConnections.append(handler);

            qDebug() << "RMO TCP connected, descriptor:"
                     << client->socketDescriptor()
                     << "total clients:" << m_rmoConnections.size();
        }
    }
}

void TcpHandler::onError(QAbstractSocket::SocketError error) {
    if(error) {
        emit errorOccurred(m_socket->errorString());
        LOG(InternalMessageLog(CLASS_NAME, LogLevel::Error, "NOTIFY NOT SENDED",
        "ERROR OCCURED WHILE SENDING MESSAGE; REASON LISTED BELOW:\n"
                                   + m_socket->errorString()));
    }
}

void TcpHandler::onReadyRead() {
    LOG_INTERNAL(CLASS_NAME, "СОКЕТ TCP", "Готов считывать сообщения");
    m_buffer.append(m_socket->readAll());
    processBuffer(m_buffer, [this](const QByteArray& data) {
        processIncomingMessage(data);
    });
}

template<typename F>
void TcpHandler::processBuffer(QByteArray& buffer, F processFunc) {
    int pos = 0;
    const int minHeader = static_cast<int>(sizeof(TcpHeader));
    const int maxPacket = 4096; // Максимальный допустимый размер пакета

    while (buffer.size() - pos >= minHeader) {
        // 1. Пробуем прочитать заголовок с текущей позиции
        TcpHeader header;
        memcpy(&header, buffer.constData() + pos, minHeader);

        // 2. Проверяем контрольную сумму
        if (!header.isValid()) {
            // Не валидный заголовок — сдвигаем окно поиска на 1 байт
            ++pos;
            continue;
        }

        // 3. Проверяем размер пакета
        uint32_t fullMessageSize = minHeader + header.sizeData;
        if (header.sizeData > maxPacket) {
            // sizeData неадекватен — сдвигаем окно поиска на 1 байт
            ++pos;
            continue;
        }

        if (buffer.size() - pos < static_cast<int>(fullMessageSize)) {
            // Ждём остальные байты
            break;
        }

        // 4. Всё ок — извлекаем пакет
        QByteArray messageData = buffer.mid(pos, fullMessageSize);

        // qDebug() << "Buffer size:" << buffer.size()
        //          << "\n\tpos:" << pos << "\n";
        // qDebug() << "Header type:" << header.typePack
        //          << "\n\tsizeData:" << header.sizeData
        //          << "\n\tchecksum:" << header.checkSum
        //          << "\n\tisValid:" << header.isValid() << "\n";
        processFunc(messageData);

        // 5. Сдвигаем позицию на размер пакета
        pos += fullMessageSize;
    }

    // 6. Удаляем обработанные байты из буфера
    if (pos > 0)
        buffer.remove(0, pos);
}

void TcpHandler::sendMessage(const QByteArray& data) {
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->write(data);
    }
}

void TcpHandler::sendRmoMessage(int idx, const RmoBaseMessage& msg) {
    if (idx >= 0 && idx < m_rmoConnections.size()) {
        m_rmoConnections[idx]->sendData(msg.toByteArray());
    }
}

void TcpHandler::sendRmoMessageToAll(const RmoBaseMessage& msg) {
    for (auto* handler : m_rmoConnections) {
        handler->sendData(msg.toByteArray());
    }
}

void TcpHandler::handleRegistration() {
    if (!m_isRegistered) {
        auto msg_0x01 = MessageFactory::create<TcpMessage_0x01>();
        QByteArray data = msg_0x01.serializeFull();
        sendMessage(data);
        LOG(ProtocolMessageLog(CLASS_NAME, LogLevel::MessageSent,
                                Protocol::TCP, data,
                                Logger::jobj_toString(msg_0x01.to_json())));
    }
}

void TcpHandler::sendPeriodicMessages() {
    if (m_isRegistered) {
        // Отправка 0x03 (Запрос времени)
        auto msg_0x03 = MessageFactory::create<TcpMessage_0x03>();
#ifdef QT_DEBUG
        std::cout << "Sent message 0x03:\n";
#endif
#ifdef TRACE
        qDebug() << "time_now =" << TimeProvider::instance().getMs();
        std::cout << msg_0x03.toString()
                  << std::endl << "\n";
#endif
        sendMessage(msg_0x03.serializeFull());
        LOG(ProtocolMessageLog(CLASS_NAME, LogLevel::MessageSent,
                                Protocol::TCP, msg_0x03.serializeFull(),
                                Logger::jobj_toString(msg_0x03.to_json())));
        // !!!!Отправка 0x24 (Статус модуля)
        auto msg_0x24 = MessageFactory::create<TcpMessage_0x24>();
#ifdef QT_DEBUG
        std::cout << "Sent message 0x24:\n";
#endif
#ifdef TRACE
        std::cout << msg_0x24.toString()
                  << std::endl << "\n";
#endif
        sendMessage(msg_0x24.serializeFull());
        LOG(ProtocolMessageLog(CLASS_NAME, LogLevel::MessageSent,
                                Protocol::TCP, msg_0x24.serializeFull(),
                                Logger::jobj_toString(msg_0x24.to_json())));
        // TcpHandler::sendModuleStatus();

        // Отправка 0x1021 (Текущее состояние РЛС-Н)
        auto msg_0x1021_1 = MessageFactory
            ::create<TcpMessage_0x1021>(AppConfig::ModulePPU_num
                                                    ::ModulePPU_num_RLMN_1),
            msg_0x1021_2 = MessageFactory
                ::create<TcpMessage_0x1021>(AppConfig::ModulePPU_num
                                                        ::ModulePPU_num_RLMN_2);
#ifdef QT_DEBUG
        std::cout << "Sent message 0x1021_1:\n";
        std::cout << "Sent message 0x1021_2:\n";
#endif
#ifdef TRACE
        std::cout << msg_0x1021_1.toString()
                  << std::endl << "\n";

        std::cout << msg_0x1021_2.toString()
                  << std::endl << "\n";
#endif
        sendMessage(msg_0x1021_1.serializeFull());
        LOG_PROTOCOL(CLASS_NAME, LogLevel::MessageSent, Protocol::TCP, 
                                msg_0x1021_1.serializeFull(),
                                Logger::jobj_toString(msg_0x1021_1.to_json()));
            
        sendMessage(msg_0x1021_2.serializeFull());
        LOG_PROTOCOL(CLASS_NAME, LogLevel::MessageSent, Protocol::TCP, 
                                msg_0x1021_2.serializeFull(),
                                Logger::jobj_toString(msg_0x1021_2.to_json()));
    }
}

void TcpHandler::processIncomingMessage(const QByteArray &data) {
    static int isRemoteCnt = 1;
    auto parsed = MessageFactory::parse(data);
    if (parsed) {
#ifdef QT_DEBUG
        std::cout << "Received message.\n";
#endif
#ifdef TRACE
        std::cout << MessageFactory::messageToString(parsed)
                  << "\n" << std::endl << "\n";
#endif
        switch (static_cast<PBU_MessageNumbers>(parsed->typePack)) {
        case PBU_MessageNumbers::RES_REGISTRATION: {
            auto msg_0x02 = reinterpret_cast<TcpMessage_0x02*>(parsed.get());
            LOG(ProtocolMessageLog(CLASS_NAME, LogLevel::MessageReceived,
                                   Protocol::TCP, data,
                                   Logger::jobj_toString(msg_0x02->to_json())));
            if (msg_0x02->errorConnect == 0) {
#ifdef QT_DEBUG
                std::cout << "Module registered using message 0x02."
                          << std::endl << "\n";
#endif
                AppConfig::instance()
                    .SET_PBU_PROTO_HEAD_IDX_MODULE(msg_0x02->idxModule);

                m_isRegistered = true;
                m_registrationTimer->stop();

                auto msg_0x05 = MessageFactory::create<TcpMessage_0x05>();

                sendMessage(msg_0x05.serializeFull());
                LOG(ProtocolMessageLog(CLASS_NAME, LogLevel::MessageSent,
                                Protocol::TCP, msg_0x05.serializeFull(),
                                Logger::jobj_toString(msg_0x05.to_json())));
#ifdef QT_DEBUG
                std::cout << "Sent message 0x05:\n";
#endif
#ifdef TRACE
                std::cout << msg_0x05.toString()
                          << std::endl << "\n";
#endif
                std::vector<TcpMessage_0x1023::ModuleCapability> capabilities {
                    TcpMessage_0x1023::ModuleCapability(AppConfig::ModulePPU_num
                                                        ::ModulePPU_num_RLMN_1),
                    TcpMessage_0x1023::ModuleCapability(AppConfig::ModulePPU_num
                                                        ::ModulePPU_num_RLMN_2),
                };
                auto msg_0x1023 = MessageFactory
                    ::create<TcpMessage_0x1023>(capabilities);

                sendMessage(msg_0x1023.serializeFull());
                LOG(ProtocolMessageLog(CLASS_NAME, LogLevel::MessageSent,
                                Protocol::TCP, msg_0x1023.serializeFull(),
                                Logger::jobj_toString(msg_0x1023.to_json())));
#ifdef QT_DEBUG
                std::cout << "Sent message 0x1023:\n";
#endif
#ifdef TRACE
                std::cout << msg_0x1023.toString()
                          << std::endl << "\n";
#endif
                m_periodicTimer->start(900); // Периодическая отправка сообщений
            }
            break;
        }
        case PBU_MessageNumbers::TIMESTAMP: {
            auto msg_0x04 = reinterpret_cast<TcpMessage_0x04*>(parsed.get());
            LOG(ProtocolMessageLog(CLASS_NAME, LogLevel::MessageReceived,
                Protocol::TCP, data,
                Logger::jobj_toString(msg_0x04->to_json())));
                TimeProvider::instance().syncTimePBU(msg_0x04->timeRequest1,
                                     msg_0x04->timeRequest2,
                                     msg_0x04->timeAnswer1,
                                     msg_0x04->timeAnswer2);
#ifdef QT_DEBUG
            std::cout << "Time synchronized using message 0x04."
                      << std::endl << "\n";
#endif
#ifdef TRACE
            qDebug() << "0x04. time_now =" << TimeProvider::instance().getMs();
#endif
            // Инкремент счётчика для отмены признака внешнего уравления
            isRemoteCnt++;
            break;
        }
        case PBU_MessageNumbers::TARGET_DESIGNATION: {
            auto msg_0x641 = reinterpret_cast<TcpMessage_0x641*>(parsed.get());
            LOG(ProtocolMessageLog(CLASS_NAME, LogLevel::MessageReceived,
                                Protocol::TCP, data,
                                Logger::jobj_toString(msg_0x641->to_json())));

            AppConfig::instance().SET_PBU_PROTO_0X24_MODE(1024);
            AppConfig::instance().SET_PBU_PROTO_0X23_ID_COM(
                to_underlying(PBU_MessageNumbers::TARGET_DESIGNATION));
            AppConfig::instance().SET_PBU_PROTO_0X23_CODE(
                to_underlying(AppConfig::ReceiptCodes::IN_PROGRESS));

            // Если пришла отмена ЦУ, то сбросить
            // признак внешнего управления
            if (msg_0x641->typeCeil == 3) {
                isRemoteCnt = 100;
                AppConfig::instance().setModulePPU_isRemote(
                            AppConfig::instance().ModulePPU_num_RLMN_1,
                            AppConfig::instance().ModulePPU_isRemote_defaulted);
            } else {
                isRemoteCnt = 0;
            }

            auto msg_0x23 = MessageFactory::create<TcpMessage_0x23>();
#ifdef QT_DEBUG
            std::cout << "Sent message 0x23:\n";
#endif
#ifdef TRACE
            std::cout << msg_0x23.toString()
                      << std::endl << "\n";
#endif
            sendMessage(msg_0x23.serializeFull());
            LOG(ProtocolMessageLog(CLASS_NAME, LogLevel::MessageSent,
                                Protocol::TCP, msg_0x23.serializeFull(),
                                Logger::jobj_toString(msg_0x23.to_json())));

            auto msg_0x24 = MessageFactory::create<TcpMessage_0x24>();
#ifdef QT_DEBUG
            std::cout << "Sent message 0x24:\n";
#endif
#ifdef TRACE
            std::cout << msg_0x24.toString()
                      << std::endl << "\n";
#endif
            sendMessage(msg_0x24.serializeFull());
            LOG(ProtocolMessageLog(CLASS_NAME, LogLevel::MessageSent,
                Protocol::TCP, msg_0x24.serializeFull(),
                Logger::jobj_toString(msg_0x24.to_json())));

            // Задать ЦУ для ППУ
            AppConfig::instance().setModulePPU_CU(
                        AppConfig::instance().ModulePPU_num_RLMN_1,
                        static_cast<double>(msg_0x641->ceilAz),
                        static_cast<double>(msg_0x641->ceilElev),
                        static_cast<double>(msg_0x641->ceilVAz),
                        static_cast<double>(msg_0x641->ceilVElev));

            // Определяем, насколько текущие координаты ППУ отличаются от ЦУ
            double dAz = AppConfig::instance().getModulePPU_az(
                        AppConfig::instance().ModulePPU_num_RLMN_1)
                    - static_cast<double>(msg_0x641->ceilAz);
            double dAn = AppConfig::instance().getModulePPU_an(
                        AppConfig::instance().ModulePPU_num_RLMN_1)
                    - static_cast<double>(msg_0x641->ceilElev);

            // Если координаты отличаются незначительно, то отправляем ответ
            if (abs(dAz) < 0.6 && abs(dAn) < 0.6)
            {
                auto msg_0x1022 = MessageFactory::
                            create<TcpMessage_0x1022>(nullptr, 0, 0);
                // auto msg_0x1022 = MessageFactory::
                //          create<TcpMessage_0x1022>(nullptr);

                msg_0x1022.trackStatus = 0;
                msg_0x1022.isExtra = 0;
                msg_0x1022.isTest = 0;
                msg_0x1022.snrSum = 0;
                msg_0x1022.epr = 0;
                msg_0x1022.maxRange = 20;
                msg_0x1022.numbTrack = msg_0x641->idxCeilVOI;
                msg_0x1022.rangeCnt = 0;
                msg_0x1022.speedCnt = 0;
                msg_0x1022.chanCnt = 1; /// Всегда один канал!
                msg_0x1022.rangeStart = 0;
                msg_0x1022.rangeStep = 0.1;
                msg_0x1022.vradStart = 0;
                msg_0x1022.vradStep = 0.1;

                msg_0x1022.numDrive = msg_0x641->numDrive;
                msg_0x1022.azimuth = msg_0x641->ceilAz;
                msg_0x1022.range = msg_0x641->ceilRange;
                msg_0x1022.eps = msg_0x641->ceilElev;
                msg_0x1022.vel = msg_0x641->ceilVr;
                msg_0x1022.velAz = msg_0x641->ceilVAz;
                msg_0x1022.velEps = msg_0x641->ceilVElev;
                msg_0x1022.sigAz = msg_0x641->sigAz;
                msg_0x1022.sigEps = msg_0x641->sigElev;
                msg_0x1022.sigD = msg_0x641->sigRange;
                msg_0x1022.sigVr = msg_0x641->sigVr;
                msg_0x1022.numbTrackServr = msg_0x641->idxCeilVOI;
                msg_0x1022.typeTrack =
                                static_cast<uint8_t>(msg_0x641->typeCeil);
#ifdef QT_DEBUG
                std::cout << "Sent message 0x1022:";
#endif
#ifdef TRACE
                std::cout << " " << msg_0x1022.toString() << std::endl;
#endif
                sendMessage(msg_0x1022.serializeFull());
                LOG(ProtocolMessageLog(CLASS_NAME, LogLevel::MessageSent,
                                Protocol::TCP, msg_0x1022.serializeFull(),
                                Logger::jobj_toString(msg_0x1022.to_json())));
            }

            AppConfig::instance().SET_PBU_PROTO_0X24_MODE(0);
            AppConfig::instance().SET_PBU_PROTO_0X23_CODE(
                to_underlying(AppConfig::ReceiptCodes::EXECUTED_WITHOUT_ERROR));
            auto msg_0x23_2 = MessageFactory::create<TcpMessage_0x23>();
#ifdef QT_DEBUG
            std::cout << "Sent message 0x23:\n";
#endif
#ifdef TRACE
            std::cout << msg_0x23_2.toString()
                      << std::endl << "\n";
#endif
            sendMessage(msg_0x23_2.serializeFull());
            LOG(ProtocolMessageLog(CLASS_NAME, LogLevel::MessageSent,
                                Protocol::TCP, msg_0x23_2.serializeFull(),
                                Logger::jobj_toString(msg_0x23_2.to_json())));

            auto msg_0x24_2 = MessageFactory::create<TcpMessage_0x24>();
#ifdef QT_DEBUG
            std::cout << "Sent message 0x24:\n";
#endif
#ifdef TRACE
            std::cout << msg_0x24_2.toString()
                      << std::endl << "\n";
#endif
            sendMessage(msg_0x24_2.serializeFull());
            LOG(ProtocolMessageLog(CLASS_NAME, LogLevel::MessageSent,
                                Protocol::TCP, msg_0x24_2.serializeFull(),
                                Logger::jobj_toString(msg_0x24_2.to_json())));
            break;
        }
        case PBU_MessageNumbers::TEXT_FOR_OPERATOR_PBU: {
            auto msg_0x45 = reinterpret_cast<TcpMessage_0x45*>(parsed.get());
            LOG(ProtocolMessageLog(CLASS_NAME, LogLevel::MessageReceived,
                                Protocol::TCP, data,
                                Logger::jobj_toString(msg_0x45->to_json())));
            break;
        }

        default:
#ifdef QT_DEBUG
            qDebug() << "Unknown message type received:"
                     << parsed->typePack << "\n";
#endif
            break;
        }
    } else {
#ifdef QT_DEBUG
        qDebug() << "Invalid or unknown message\n";
#endif
#ifdef TRACE
        qDebug() << Qt::endl << "ETH Message:" << data.toHex(' ').toUpper();
#endif
        LOG_INTERNAL(CLASS_NAME, "СОКЕТ TCP", "НЕИЗВЕСТНОЕ СООБЩЕНИЕ ПОЛУЧЕНО");
        LOG(ProtocolMessageLog(CLASS_NAME, LogLevel::Trace,
                               Protocol::TCP, data.toHex(' ').toUpper(),
                               ""));
    }

    // Признак внешнего управления
    if (isRemoteCnt == 0) {
        AppConfig::instance().setModulePPU_isRemote(
                    AppConfig::instance().ModulePPU_num_RLMN_1,
                    AppConfig::instance().ModulePPU_isRemote_search);
    }
    if (isRemoteCnt > 10) {
        AppConfig::instance().setModulePPU_isRemote(
                    AppConfig::instance().ModulePPU_num_RLMN_1,
                    AppConfig::instance().ModulePPU_isRemote_defaulted);
    }
}

// Универсальная обработка сообщений от РМО (TCP)
void TcpHandler::processIncomingMessageRMO(const QByteArray &data) {
#ifdef TRACE
    qDebug() << Qt::endl << "RMO TCP Message:" << data.toHex(' ').toUpper();
#endif
    if (data.size() < static_cast<int>(sizeof(int32_t))) return;
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::LittleEndian);
    int32_t type = 0;
    stream >> type;

    // Передаём ВСЮ data (включая type) в фабрику
    auto msg = RmoMessageFactory::instance().fromByteArray(type, data);
    if (msg) {
        if (type == static_cast<int32_t>(RmoMsgType::TARGET_PPU)) {
            auto* typedMsg = dynamic_cast<RmoMessage_TargetPPU*>(msg.get());
            if (typedMsg) {
                // Используйте поля typedMsg->azPPU, ...
                AppConfig::instance().setModulePPU_CU(
                    AppConfig::instance().ModulePPU_num_RLMN_1,
                    typedMsg->azTarget, typedMsg->anTarget,
                    typedMsg->azTargetVel, typedMsg->anTargetVel
                );
            }
        }
        // else if (type == ...) // другие типы сообщений
    }
}

// --- WebSocket (браузерные РМО) ---

void TcpHandler::onWebSocketNewConnection() {
    QWebSocket* ws = m_wsServer->nextPendingConnection();
    if (!ws) return;
    m_wsClients.append(ws);
    qDebug() << "RMO WebSocket connected:" << ws->peerAddress().toString();

    connect(ws, &QWebSocket::textMessageReceived, this,
                                    [this, ws](const QString& message) {
        onWebSocketTextMessageReceived(message, ws);
    });
    connect(ws, &QWebSocket::disconnected, this, [this, ws]() {
        m_wsClients.removeOne(ws);
        ws->deleteLater();
        qDebug() << "RMO WebSocket disconnected";
    });
}

void TcpHandler::onWebSocketTextMessageReceived(const QString& message,
                                                QWebSocket* ws) {
    Q_UNUSED(message);
    Q_UNUSED(ws);
    // try {
    //     auto obj = nlohmann::json::parse(message.toStdString());
    //     double azTarget = obj.value("azTarget", 0.0);
    //     double anTarget = obj.value("anTarget", 0.0);
    //     double azTargetVel = obj.value("azTargetVel", 0.0);
    //     double anTargetVel = obj.value("anTargetVel", 0.0);
    //     AppConfig::instance().setModulePPU_CU(
    //         AppConfig::instance().ModulePPU_num_RLMN_1,
    //         azTarget, anTarget, azTargetVel, anTargetVel);

    //     nlohmann::json reply = {{"status", "ok"}};
    //     sendRmoMessageJSON(ws, reply);
    // } catch (const std::exception& e) {
    //     nlohmann::json reply = {{"status", "error"}, {"reason", e.what()}};
    //     sendRmoMessageJSON(ws, reply);
    // }
}

void TcpHandler::sendRmoMessageJSON(QWebSocket* ws, const RmoBaseMessage& msg) {
    if (ws && ws->state() == QAbstractSocket::ConnectedState) {
        ws->sendTextMessage(QString::fromStdString(msg.toJson().dump()));
    }
}

void TcpHandler::sendRmoMessageToAllJSON(const RmoBaseMessage& msg) {
    QString msg_string = QString::fromStdString(msg.toJson().dump());
    for (QWebSocket* ws : m_wsClients) {
        if (ws->state() == QAbstractSocket::ConnectedState) {
            ws->sendTextMessage(msg_string);
        }
    }
}
