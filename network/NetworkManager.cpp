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

void NetworkManager::onEthReceived(const Message& msg) {
    qDebug() << "ETH Message:" << msg.payload;

}
