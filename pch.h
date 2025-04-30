//Прекомпилированные заголовки, включаются везде
// #include <QThread>
// #include <QMutex>
// #include <QWaitCondition>
// #include <QQueue>
// #include <QSqlDatabase>                     всё это из Логгера
// #include <QSqlQuery>
// #include <QDateTime>
// #include <QString>
// #include <atomic>
// #include <vector>
// защитник от множественногот включения
#pragma once
#include <utils/time_provider/TimeProvider.h>
#include "utils/AppConfig.h"
#include "core/logger/Logger.h"



/*
Если хочется без указания типа класса, где был вызов, передавать *this в typeid
*/

/**
 * @def LOG(logObject)
 * @brief Универсальный макрос для логирования
 * @param logObject Допустимые типы: InitLog, InternalMessageLog, ProtocolMessageLog
 * @example LOG(InitLog(typeid(this).name(), StatusInit::LOADED))
 * @example LOG(InitLog(typeid(this).name(), LogLevel::Info, StatusInit::LOADED))
 * @example LOG(InternalMessageLog(typeid(this).name(), "message type", "payloadPart Here"))
 * @example LOG(InternalMessageLog(typeid(this).name(), LogLevel::Info,  "message type", "payloadPart Here"))
 * @example LOG(ProtocolMessageLog(typeid(this).name(), Protocol::TCP, QByteArray, "{json_data}"))
 * @example LOG(ProtocolMessageLog(typeid(this).name(), LogLevel::Info, Protocol::TCP, QByteArray, "{json_data}"))
 */
#define LOG(logObject) Logger::instance().log(logObject)

/**
 * @def LOG_INIT(source, ...)
 * @brief Логирование событий инициализации
 * @param component Имя компонента (QString)
 * @param ... Варианты:
 *   - StatusInit status                      : статус инициализации \n
 *   - LogLevel level, StatusInit status     : уровень при логировании сбщ. и статус
 * @example LOG_INIT(typeid(this).name(), StatusInit::LOADED)
 * @example LOG_INIT(typeid(this).name(), LogLevel::Info, StatusInit::NOT_LOADED)
 */
#define LOG_INIT(source, ...) \
    Logger::instance().log(InitLog(source, ##__VA_ARGS__))

/**
 * @def LOG_INTERNAL(source, ...)
 * @brief Логирование внутренних сообщений
 * @param source Источник сообщения (QString)
 * @param ... Варианты:
 *   - QString parsed       : парсированные данные \n
 *   - LogLevel level, QString parsed : с указанием уровня
 * @example LOG_INTERNAL(typeid(this).name(), "заголовок, или тип сообщения" ,"структура, обработанная через toJSON")
 * @example LOG_INTERNAL(typeid(this).name(), LogLevel::Debug, "заголовок, или тип сообщения", "структура, обработанная через toJSON")
 */
#define LOG_INTERNAL(source, ...) \
    Logger::instance().log(InternalMessageLog(source, ##__VA_ARGS__))

/**
 * @def LOG_PROTOCOL(source, ...)
 * @brief Логирование протокольных сообщений
 * @param source Источник сообщения (QString)
 * @param ... Варианты:
 *   - QString protocol, QByteArray rawData, QString jsonData      : тип протокола, сырые данные, данные в ф-те json \n
 *   - LogLevel level, QString protocol, QByteArray rawData, QString jsonData    : с указанием уровня
 * @example LOG_PROTOCOL(typeid(this).name(), "TCP", "HEX DATA", "JSON data")
 * @example LOG_PROTOCOL(typeid(this).name(), LogLevel::Info, "MAC", "HEX DATA", "JSON data")
 */
#define LOG_PROTOCOL(source, ...) \
    Logger::instance().log(ProtocolMessageLog(source, ##__VA_ARGS__))

