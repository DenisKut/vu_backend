#ifndef LOGGER_MESSAGES_H
#define LOGGER_MESSAGES_H
#include <QString>
#include <utils/time_provider/TimeProvider.h>
#include "json/json.h"
//#инклюд остальных структуры
#include "network/messages/EthMessage.h"
#include "network/messages/TcpMessage.h"
#include "network/messages/Message.h"


QString toJSON();
QString fromJSON();

#endif
