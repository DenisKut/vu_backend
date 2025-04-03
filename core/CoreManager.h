#ifndef COREMANAGER_H
#define COREMANAGER_H
// Класс отвечает за инициализацию всех компонентов
// и за управление жизненным циклом приложения
#pragma once

#include <QObject>
#include "network/NetworkManager.h"
#include "utils/time_provider/TimeProvider.h"
#include "core/logger/Logger.h"

class CoreManager : public QObject {
    Q_OBJECT
public:
    explicit CoreManager(QObject* parent = nullptr);
    void initialize();

private:
    TimeProvider* m_timeProvider;
    NetworkManager* m_networkManager;
};

#endif // COREMANAGER_H
