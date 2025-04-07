#ifndef MESSAGE_H
#define MESSAGE_H

#include <QByteArray>

struct Message {
    quint64 timestamp;
    QByteArray payload;
};

#endif // MESSAGE_H
