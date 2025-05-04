// CoreManager.cpp
#include "CoreManager.h"
#include "network/NetworkManager.h"

CoreManager::CoreManager(QObject* parent) : QObject(parent) {
    m_networkManager = new NetworkManager(this);
}

void CoreManager::initialize() {
    m_networkManager->start(); // Запуск сетевого модуля
}
