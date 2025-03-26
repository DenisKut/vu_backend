#ifndef ETHHANDLER_H
#define ETHHANDLER_H

#include <QTimer>
#include <QObject>
#include <QDataStream>
#include "../messages/Message.h"
#include "utils/TimeProvider.h"

class MacHandler : public QObject {
    Q_OBJECT

public:
    explicit MacHandler(TimeProvider* timeProvider, QObject* parent = nullptr);
    ~MacHandler();

    void messageProccess(const Message& msg); // Обработчик принятых MAC пакетов

public slots:
    void checkReceived(); // Проверка сокета на наличие принятых MAC пакетов

signals:
    void macTimerReceiveRestart(); // Запуск таймера вызова обработчика принятых MAC пакетов

private:
    TimeProvider* m_timeProvider;
};

#endif // ETHHANDLER_H
