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
#ifndef PCH_H
#define PCH_H
#include "utils/time_provider/TimeProvider.h"
#include "utils/AppConfig.h"
#include "common/logger/Logger.h"
#include <QDebug>
/*
Если хочется без указания типа класса, где был вызов, передавать *this в typeid
*/

/**
 * @def LOG(logObject)
 * @brief Универсальный макрос для логирования
 * @param logObject Допустимые типы: InitLog, InternalMessageLog, ProtocolMessageLog
 * @example LOG(InitLog(CLASS_NAME, StatusInit::LOADED))
 * @example LOG(InitLog(CLASS_NAME, LogLevel::Info, StatusInit::LOADED))
 * @example LOG(InternalMessageLog(CLASS_NAME, "message type", "payloadPart Here"))
 * @example LOG(InternalMessageLog(CLASS_NAME, LogLevel::Info,  "message type", "payloadPart Here"))
 * @example LOG(ProtocolMessageLog(CLASS_NAME, Protocol::TCP, QByteArray, "{json_data}"))
 * @example LOG(ProtocolMessageLog(CLASS_NAME, LogLevel::Info, Protocol::TCP, QByteArray, "{json_data}"))
 */
#define LOG(logObject) Logger::instance().log(logObject)

/**
 * @def LOG_INIT(source, ...)
 * @brief Логирование событий инициализации
 * @param component Имя компонента (QString)
 * @param ... Варианты:
 *   - StatusInit status                      : статус инициализации \n
 *   - LogLevel level, StatusInit status     : уровень при логировании сбщ. и статус
 * @example LOG_INIT(CLASS_NAME, StatusInit::LOADED)
 * @example LOG_INIT(CLASS_NAME, LogLevel::Info, StatusInit::NOT_LOADED)
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
 * @example LOG_INTERNAL(CLASS_NAME, "заголовок, или тип сообщения" ,"структура, обработанная через toJSON")
 * @example LOG_INTERNAL(CLASS_NAME, LogLevel::Debug, "заголовок, или тип сообщения", "структура, обработанная через toJSON")
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
 * @example LOG_PROTOCOL(CLASS_NAME, "TCP", "HEX DATA", "JSON data")
 * @example LOG_PROTOCOL(CLASS_NAME, LogLevel::Info, "MAC", "HEX DATA", "JSON data")
 */
#define LOG_PROTOCOL(source, ...) \
    Logger::instance().log(ProtocolMessageLog(source, ##__VA_ARGS__))



namespace Utils {
    QString demangleClassName(const char* name);
}

#define CLASS_NAME Utils::demangleClassName(typeid(*this).name())



#endif
