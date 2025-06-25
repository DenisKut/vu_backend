// CoreManager.cpp
#include "CoreManager.h"
#include "network/NetworkManager.h"
#include "calculator/Calculator.h"

CoreManager::CoreManager(QObject *parent) : QObject(parent)
{
	m_networkManager = new NetworkManager(this);
}

void CoreManager::initialize()
{
	try
	{
		m_networkManager->start(); // Запуск сетевого модуля
		// Пример: отправить paramCU из Calculator в EthHandler
		Calculator::instance()->setTestToETHCallback(
			[this](const Calculator::testToETH &test)
			{
			// Например, вызвать метод EthHandler (через NetworkManager)
			if (m_networkManager->m_ethHandler) {
				// Если EthHandler потоковый — используйте QMetaObject::invokeMethod
				QMetaObject::invokeMethod(m_networkManager->m_ethHandler, [test]()
				{
					// Здесь можно обработать testToETH и отправить его в EthHandler
					// Лучше написать в EthHandler метод для обработки testToETH и вызвать его здесь
					//
					// Соответственно внутри тела функции EthHandler должен быть
					// реализован метод для обработки testToETH и передачи данных далее
					// m_ethHandler->processTest(test);
				},
				Qt::QueuedConnection);
		} });

		Calculator::instance()->setTestToUDPCallback(
			[this](const Calculator::testToUDP &test)
			{
			// Например, вызвать метод UdpHandler (через NetworkManager)
			if (m_networkManager->m_udpHandler) {
				QMetaObject::invokeMethod(m_networkManager->m_udpHandler, [test]()
				{
					// По аналогии с примером выше
					// m_udpHandler->processTest(test);
				},
				Qt::QueuedConnection);
		} });
		Calculator::instance()->setTestToTCPCallback(
			[this](const Calculator::testToTCP &test)
			{
			// Например, вызвать метод TcpHandler (через NetworkManager)
			if (m_networkManager->m_tcpHandler) {
				QMetaObject::invokeMethod(m_networkManager->m_tcpHandler, [test]()
				{
					// По аналогии с примером выше
					// m_tcpHandler->processTest(test);
				},
				Qt::QueuedConnection);
		} });
	}
	catch (...)
	{
		LOG_INIT(CLASS_NAME, StatusInit::NOT_LOADED__ERROR_CRITICAL);
	}
}
