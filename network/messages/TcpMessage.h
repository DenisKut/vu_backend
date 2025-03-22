#ifndef TCPMESSAGE_H
#define TCPMESSAGE_H
#pragma once
#include <QByteArray>

struct TcpMessage {
    quint64 timestamp;
    QByteArray payload;
};
#endif // TCPMESSAGE_H
