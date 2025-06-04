#ifndef LOGGERMACROS_H
#define LOGGERMACROS_H
#include "LogManager.h"
#include "messageLogs/InternalMessageLog.h"
#include "messageLogs/ProtocolMessageLog.h"
#include "messageLogs/InitLog.h"


// Макросы для быстрого логирования
#define LOG_INIT(...)   LogManager::instance().log<InitLog>(__VA_ARGS__)
#define LOG_INTERNAL(...) LogManager::instance().log<InternalMessageLog>(__VA_ARGS__)
#define LOG_PROTOCOL(...) LogManager::instance().log<ProtocolMessageLog>(__VA_ARGS__)

// Универсальный макрос
#define LOG(type, ...) LogManager::instance().log<type>(__VA_ARGS__)
#endif // LOGGERMACROS_H
