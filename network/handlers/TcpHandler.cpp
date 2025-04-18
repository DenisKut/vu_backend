#include "TcpHandler.h"
#include <QHostAddress>

TcpHandler::TcpHandler(TimeProvider* timeProvider, QObject* parent)
    : QObject(parent), m_timeProvider(timeProvider), m_socket(new QTcpSocket(this))
{
    m_socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    connect(m_socket, &QTcpSocket::connected, this, &TcpHandler::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpHandler::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpHandler::onReadyRead);

    connect(&m_registrationTimer, &QTimer::timeout, this, &TcpHandler::handleRegistration);
    connect(&m_periodicTimer, &QTimer::timeout, this, &TcpHandler::sendPeriodicMessages);
}

TcpHandler::~TcpHandler() {
    disconnect();
}

void TcpHandler::connectToHost(const QString& host, quint16 port) {
    m_socket->connectToHost(host, port, QIODevice::ReadWrite, QAbstractSocket::AnyIPProtocol);
}

void TcpHandler::disconnect() {
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
    }
    m_registrationTimer.stop();
    m_periodicTimer.stop();
}

void TcpHandler::onConnected() {
    qDebug() << "Connected to TCP host";
    m_registrationTimer.start(1000); // Отправка 0x01 каждую секунду
}

void TcpHandler::onDisconnected() {
    qDebug() << "Disconnected from TCP host";
    m_isRegistered = false;
    m_registrationTimer.stop();
    m_periodicTimer.stop();
}

void TcpHandler::onError(QAbstractSocket::SocketError error) {
    emit errorOccurred(m_socket->errorString());
}

void TcpHandler::onReadyRead() {
    while (m_socket->bytesAvailable() > 0) {
        QByteArray data = m_socket->readAll();
        if (data.size() >= static_cast<int>(sizeof(TcpHeader))) {
            TcpHeader header = TcpHeader::deserialize(data);
            // processIncomingMessage(header);
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
        sendMessage(msg_0x03.serializeFull());

        // !!!!Отправка 0x24 (Статус модуля)
        auto msg_0x24 = MessageFactory::create<TcpMessage_0x24>();
        sendMessage(msg_0x24.serializeFull());
        // TcpHandler::sendModuleStatus();

        // Отправка 0x1021 (Текущее состояние РЛС-Н)
        auto msg_0x1021_1 = MessageFactory::create<TcpMessage_0x1021>(),
            msg_0x1021_2 = MessageFactory::create<TcpMessage_0x1021>();
        sendMessage(msg_0x1021_1.serializeFull());
        sendMessage(msg_0x1021_2.serializeFull());
    }
}

// void TcpHandler::processIncomingMessage(const TcpHeader& msg) {
//     switch (static_cast<PBU_MessageNumbers>(msg.typePack)) {
//     case PBU_MessageNumbers::RES_REGISTRATION: {
//         m_isRegistered = true;
//         m_registrationTimer.stop();
//         m_periodicTimer.start(1000); // Периодическая отправка сообщений
//         break;
//     }
//     case PBU_MessageNumbers::TIMESTAMP: {
//         TcpMessage_0x04* timeMsg = reinterpret_cast<TcpMessage_0x04*>(&msg);
//         m_timeProvider->syncTime(timeMsg->timeAnswer1, timeMsg->timeAnswer2);
//         qDebug() << "Time synchronized using message 0x04.";
//         break;
//     }
//     case PBU_MessageNumbers::TARGET_DESIGNATION: {
//         TcpMessage_0x641* targetMsg = reinterpret_cast<TcpMessage_0x641*>(&msg);

//         // Изменение глобального состояния
//         AppConfig::instance().setModuleState("Executing Command");

//         // Отправка подтверждения и статуса
//         TcpMessage_0x23 ackMsg;
//         TcpMessage_0x24 statusMsg;
//         sendMessage(ackMsg);
//         sendMessage(statusMsg);

//         // Формирование и отправка отметок цели
//         TcpMessage_0x1022 targetResponse;
//         targetResponse.fillFromTargetMessage(*targetMsg); // Заполнение из 0x641
//         sendMessage(targetResponse);

//         // Завершение обработки
//         AppConfig::instance().SET_MODULE_MODE(1024);
//         TcpMessage_0x23 finalAckMsg;
//         sendMessage(finalAckMsg);
//         break;
//     }
//     default:
//         qDebug() << "Unknown message type received:" << msg.typePack;
//         break;
//     }
// }
