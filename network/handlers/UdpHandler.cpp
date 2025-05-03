#include "UdpHandler.h"
#include "utils/AppConfig.h"
#include <QNetworkDatagram>

UdpHandler::UdpHandler(TimeProvider* timeProvider, QObject* parent)
    : QObject(parent),
    m_timeProvider(timeProvider),
    m_running(false) {
}


UdpHandler::~UdpHandler() { stop(); }

void UdpHandler::start(int port) {
    if (m_running) return;
    m_port = port;
    m_running = true;
    m_thread = std::thread([this]() { this->run(); });
}

void  UdpHandler::stop() {
    m_running = false;
    if (m_thread.joinable()) m_thread.join();
}

void UdpHandler::sendResponse(const QByteArray& data,
                  const QHostAddress& addr,
                  quint16 port) {
    if (m_socket) m_socket->writeDatagram(data, addr, port);
}

void UdpHandler::run() {
    qDebug() << "UDP started";
    QUdpSocket socket;
    m_socket = &socket;
    socket.bind(QHostAddress::Any, m_port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);

    char buf[1024]; // Сюда фиксированный размер буфера
    while (m_running) {
        if (socket.waitForReadyRead(1)) {
            while (socket.hasPendingDatagrams()) {
                QHostAddress sender;
                quint16 senderPort;
                qint64 len = socket.readDatagram(buf, sizeof(buf), &sender, &senderPort);
                if (len > 0) {
                    QByteArray data(buf, int(len));
                    emit packetReceived(data, sender, senderPort);
                    // Здесь можно сразу разобрать пакет и отправить ответ:
                    // QByteArray response = processPacket(data);
                    // sendResponse(response, sender, senderPort);
                }
            }
        }
    }
    m_socket = nullptr;
}
