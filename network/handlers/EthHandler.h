#ifndef ETHHANDLER_H
#define ETHHANDLER_H

#include <QObject>
#include <QDataStream>
#include "../messages/Message.h"

class EthHandler : public QObject {
    Q_OBJECT

public:
    EthHandler();

public slots:
    void ethFrameReceived(const Message& msg); // Обработчик принятых MAC пакетов
};

#endif // ETHHANDLER_H
