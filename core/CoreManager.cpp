// CoreManager.cpp
#include "CoreManager.h"
#include "network/NetworkManager.h"
#include "utils/TimeProvider.h"

CoreManager::CoreManager(QObject* parent) : QObject(parent) {
    m_timeProvider = new TimeProvider(this);
    m_networkManager = new NetworkManager(m_timeProvider, this);
}

void CoreManager::initialize() {
    m_networkManager->start(); // Запуск сетевого модуля
}
