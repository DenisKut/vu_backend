#ifndef COREMANAGER_H
#define COREMANAGER_H
// Класс отвечает за инициализацию всех компонентов
// и за управление жизненным циклом приложения
#pragma once

#include <QObject>
#include "network/NetworkManager.h"

class CoreManager : public QObject {
    Q_OBJECT
public:
    explicit CoreManager(QObject* parent = nullptr);
    void initialize();

private:
    NetworkManager* m_networkManager;
};

#endif // COREMANAGER_H
