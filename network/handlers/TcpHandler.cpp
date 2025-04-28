#include "TcpHandler.h"
#include <QHostAddress>

TcpHandler::TcpHandler(TimeProvider* timeProvider, QObject* parent)
    : QObject(parent), m_timeProvider(timeProvider),
        m_socket(new QTcpSocket(this))
{
    m_socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    connect(m_socket, &QTcpSocket::connected, this, &TcpHandler::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this,
            &TcpHandler::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpHandler::onReadyRead);

    connect(&m_registrationTimer, &QTimer::timeout, this,
            &TcpHandler::handleRegistration);
    connect(&m_periodicTimer, &QTimer::timeout, this,
            &TcpHandler::sendPeriodicMessages);
}

TcpHandler::~TcpHandler() {
    disconnect();
}

void TcpHandler::connectToHost(const QString& host, quint16 port) {
    m_socket->connectToHost(host, port, QIODevice::ReadWrite,
                                            QAbstractSocket::AnyIPProtocol);
}

void TcpHandler::disconnect() {
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
    }
    m_registrationTimer.stop();
    m_periodicTimer.stop();
}

void TcpHandler::onConnected() {
    std::cout << "Connected to TCP host" << std::endl;
    m_registrationTimer.start(800); // Отправка 0x01 каждую секунду
}

void TcpHandler::onDisconnected() {
     std::cout << "Disconnected from TCP host" << std::endl;
    m_isRegistered = false;
    m_registrationTimer.stop();
    m_periodicTimer.stop();
}

void TcpHandler::onError(QAbstractSocket::SocketError error) {
    if(error)
        emit errorOccurred(m_socket->errorString());
}

void TcpHandler::onReadyRead() {
    while (m_socket->bytesAvailable() > 0) {
        QByteArray data = m_socket->readAll();
        if (data.size() >= static_cast<int>(sizeof(TcpHeader))) {
            processIncomingMessage(data);
        }
    }
}

void TcpHandler::sendMessage(const QByteArray& data) {
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->write(data);
    }
}

void TcpHandler::handleRegistration() {
    if (!m_isRegistered) {
        auto msg_0x01 = MessageFactory::create<TcpMessage_0x01>();
        QByteArray data = msg_0x01.serializeFull();
        sendMessage(data);
    }
}

void TcpHandler::sendPeriodicMessages() {
    if (m_isRegistered) {
        // Отправка 0x03 (Запрос времени)
        // TcpMessage_0x03 timeReq;
        auto msg_0x03 = MessageFactory::create<TcpMessage_0x03>();
        std::cout << "Sent message 0x03:\n"
                  << msg_0x03.toString()
                  << std::endl << "\n";
        sendMessage(msg_0x03.serializeFull());

        // !!!!Отправка 0x24 (Статус модуля)
        auto msg_0x24 = MessageFactory::create<TcpMessage_0x24>();
        std::cout << "Sent message 0x24:\n"
                  << msg_0x24.toString()
                  << std::endl << "\n";
        sendMessage(msg_0x24.serializeFull());
        // TcpHandler::sendModuleStatus();

        // Отправка 0x1021 (Текущее состояние РЛС-Н)
        auto msg_0x1021_1 = MessageFactory
            ::create<TcpMessage_0x1021>(AppConfig::ModulePPU_num
                                                    ::ModulePPU_num_RLMN_1),
            msg_0x1021_2 = MessageFactory
                ::create<TcpMessage_0x1021>(AppConfig::ModulePPU_num
                                                        ::ModulePPU_num_RLMN_2);
        std::cout << "Sent message 0x1021_1:\n"
                  << msg_0x1021_1.toString()
                  << std::endl << "\n";

        std::cout << "Sent message 0x1021_2:\n"
                  << msg_0x1021_2.toString()
                  << std::endl << "\n";

        sendMessage(msg_0x1021_1.serializeFull());
        sendMessage(msg_0x1021_2.serializeFull());
    }
}

void TcpHandler::processIncomingMessage(QByteArray &data) {;
    auto parsed = MessageFactory::parse(data);
    if(parsed) {
        std::cout << "Received message:\n"
                  << MessageFactory::messageToString(parsed)
                  << "\n" << std::endl << "\n";

        switch (static_cast<PBU_MessageNumbers>(parsed->typePack)) {
        case PBU_MessageNumbers::RES_REGISTRATION: {
            auto msg_0x02 = reinterpret_cast<TcpMessage_0x02*>(parsed.get());
            if (msg_0x02->errorConnect == 0) {
                std::cout << "Module registered using message 0x02."
                          << std::endl << "\n";
                AppConfig::instance()
                    .SET_PBU_PROTO_HEAD_IDX_MODULE(msg_0x02->idxModule);

                m_isRegistered = true;
                m_registrationTimer.stop();

                auto msg_0x05 = MessageFactory::create<TcpMessage_0x05>();

                sendMessage(msg_0x05.serializeFull());
                std::cout << "Sent message 0x05:\n"
                          << msg_0x05.toString()
                          << std::endl << "\n";

                std::vector<TcpMessage_0x1023::ModuleCapability> capabilities {
                    TcpMessage_0x1023::ModuleCapability(AppConfig::ModulePPU_num
                                                        ::ModulePPU_num_RLMN_1),
                    TcpMessage_0x1023::ModuleCapability(AppConfig::ModulePPU_num
                                                        ::ModulePPU_num_RLMN_2),
                };
                auto msg_0x1023 = MessageFactory
                    ::create<TcpMessage_0x1023>(capabilities);

                sendMessage(msg_0x1023.serializeFull());

                std::cout << "Sent message 0x1023:\n"
                          << msg_0x1023.toString()
                          << std::endl << "\n";
                m_periodicTimer.start(900); // Периодическая отправка сообщений
            }
            break;
        }
        case PBU_MessageNumbers::TIMESTAMP: {
            auto msg_0x04 = reinterpret_cast<TcpMessage_0x04*>(parsed.get());
            m_timeProvider->syncTime(msg_0x04->timeRequest1,
                                     msg_0x04->timeRequest2,
                                     msg_0x04->timeAnswer1,
                                     msg_0x04->timeAnswer2);
            std::cout << "Time synchronized using message 0x04."
                      << std::endl << "\n";
            break;
        }
        case PBU_MessageNumbers::TARGET_DESIGNATION: {
            auto msg_0x641 = reinterpret_cast<TcpMessage_0x641*>(parsed.get());

            AppConfig::instance().SET_PBU_PROTO_0X24_MODE(1024);
            AppConfig::instance().SET_PBU_PROTO_0X23_ID_COM(
                to_underlying(PBU_MessageNumbers::TARGET_DESIGNATION));
            AppConfig::instance().SET_PBU_PROTO_0X23_CODE(
                to_underlying(AppConfig::ReceiptCodes::IN_PROGRESS));

            auto msg_0x23 = MessageFactory::create<TcpMessage_0x23>();
            std::cout << "Sent message 0x23:\n"
                      << msg_0x23.toString()
                      << std::endl << "\n";
            sendMessage(msg_0x23.serializeFull());

            auto msg_0x24 = MessageFactory::create<TcpMessage_0x24>();
            std::cout << "Sent message 0x24:\n"
                      << msg_0x24.toString()
                      << std::endl << "\n";
            sendMessage(msg_0x24.serializeFull());

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
            msg_0x1022.typeTrack = static_cast<uint8_t>(msg_0x641->typeCeil);

            std::cout << "Sent message 0x1022:\n"
                      << msg_0x1022.toString()
                      << std::endl << "\n";
            sendMessage(msg_0x1022.serializeFull());

            AppConfig::instance().SET_PBU_PROTO_0X24_MODE(0);
            AppConfig::instance().SET_PBU_PROTO_0X23_CODE(
                to_underlying(AppConfig::ReceiptCodes::EXECUTED_WITHOUT_ERROR));
            auto msg_0x23_2 = MessageFactory::create<TcpMessage_0x23>();
            std::cout << "Sent message 0x23:\n"
                      << msg_0x23_2.toString()
                      << std::endl << "\n";
            sendMessage(msg_0x23_2.serializeFull());

            auto msg_0x24_2 = MessageFactory::create<TcpMessage_0x24>();
            std::cout << "Sent message 0x24:\n"
                      << msg_0x24_2.toString()
                      << std::endl << "\n";
            sendMessage(msg_0x24_2.serializeFull());
            break;
        }
        default:
            qDebug() << "Unknown message type received:"
                     << parsed->typePack << "\n";
            break;
        }
    } else {
        qDebug() << "Invalid or unknown message\n";
        qDebug() << Qt::endl << "ETH Message:" << data.toHex(' ').toUpper();
    }


}
