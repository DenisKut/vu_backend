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
        // Запуск сетевого модуля
        m_networkManager->start();

		// Пример: отправить paramCU из Calculator в EthHandler
		Calculator::instance()->setTestToETHCallback(
            [this](const Calculator::testDataToETH &testData) {
            if (m_networkManager->m_ethHandler) {
                QMetaObject::invokeMethod(m_networkManager->m_ethHandler, [testData]()
                {
                    // m_ethHandler->processTest(testData);
                },
                Qt::QueuedConnection);
            }
        });

//        // Отправить  из Calculator в EthHandler
//        Calculator::instance()->setTestToETHCallback(
//            [this](const Calculator::testDataToETH &testData) {
//            if (m_networkManager->m_ethHandler) {
//                QMetaObject::invokeMethod(m_networkManager->m_ethHandler, [testData]()
//                {
//                    // m_ethHandler->processTest(testData);
//                },
//                Qt::QueuedConnection);
//            }
//        });
	}
	catch (...)
	{
		LOG_INIT(CLASS_NAME, StatusInit::NOT_LOADED__ERROR_CRITICAL);
	}
}
