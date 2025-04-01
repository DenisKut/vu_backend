#include "NetworkManager.h"

NetworkManager::NetworkManager(TimeProvider* timeProvider, QObject* parent)
    : QObject(parent),
      m_timeProvider(timeProvider),
      m_macTimer(this),
      m_tcpHandler(timeProvider),
      m_udpHandler(timeProvider),
      m_macHandler(timeProvider)
{
    connect(&m_tcpHandler, &TcpHandler::messageReceived, this, &NetworkManager::onTcpMessage);
    connect(&m_udpHandler, &UdpHandler::datagramReceived, this, &NetworkManager::onUdpDatagram);
    connect(&m_macTimer, &QTimer::timeout, &m_macHandler, &MacHandler::checkReceived);
    connect(&m_macHandler, &MacHandler::macTimerReceiveRestart, [this](){ m_macTimer.start(1); });
}

NetworkManager::~NetworkManager() {
    stop();
}

void NetworkManager::start() {
    // m_tcpHandler.connectToHost("127.0.0.1", 12345);
    m_tcpHandler.connectToHost(
        QString::fromStdString(
            AppConfig::instance().TCP_HOST_IP()),
            AppConfig::instance().TCP_HOST_PORT()
        );
    m_udpHandler.startListening(AppConfig::instance().UDP_PORT());
    m_macHandler.start();
    m_macTimer.start(1);
}

void NetworkManager::stop() {
    m_macTimer.stop();
    m_tcpHandler.disconnect();
    m_udpHandler.stopListening();
    m_macHandler.disconnectAdapter();
}

void NetworkManager::onTcpMessage(const TcpMessage& msg) {
    qDebug() << "TCP Message:" << msg.payload;
}

void NetworkManager::onUdpDatagram(const Message& msg) {
    qDebug() << "UDP Datagram:" << msg.payload;
}
