// CoreManager.cpp
#include "CoreManager.h"
#include "network/NetworkManager.h"

CoreManager::CoreManager(QObject* parent) : QObject(parent) {
    m_networkManager = new NetworkManager(m_timeProvider, this);
}

void CoreManager::initialize() {
    try{
        m_networkManager->start(); // Запуск сетевого модуля
    }
    catch (...){
        LOG_INIT(typeid(this).name(), StatusInit::NOT_LOADED__ERROR_CRITICAL);
    }
}
