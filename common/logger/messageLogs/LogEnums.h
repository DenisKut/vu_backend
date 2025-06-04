#ifndef LOGENUMS_H
#define LOGENUMS_H
#include <QString>

enum class LogLevel {
    //mostly debug ¯\_(ツ)_/¯
    Trace,    ///> @brief детальная отладочная информация
    Debug,    ///> @brief отладочные сообщения
    //common
    Info,     ///> @brief общая информация
    Warning,  ///> @brief предупреждения
    Error,    ///> @brief ошибки
    Fatal,     ///> @brief критические ошибки
    //message in/out
    MessageReceived, ///> @brief получаемое сообщений
    MessageSent      ///> @brief принимаемое сообщение
};

QString inline logLevelToString(LogLevel level) {
    switch(level) {
    case LogLevel::Trace:           return "Trace";
    case LogLevel::Debug:           return "Debug";
    case LogLevel::Info:            return "Info";
    case LogLevel::Warning:         return "Warning";
    case LogLevel::Error:           return "Error";
    case LogLevel::Fatal:           return "Fatal";
    case LogLevel::MessageReceived: return "MessageReceived";
    case LogLevel::MessageSent:     return "MessageSent";
    default:                        return "N/A";
    }
}

/**
 * @brief приналежит InitLog
 * @details помогает разделять сообщения об инициализации компонентов по важности
 */
enum class StatusInit{
    LOADED,
    NOT_LOADED__ERROR_CRITICAL,
    NOT_LOADED
};

QString inline statusInitToString(StatusInit status) {
    switch(status) {
    case StatusInit::LOADED:                     return "LOADED";
    case StatusInit::NOT_LOADED__ERROR_CRITICAL: return "NOT_LOADED_ERROR_CRITICAL";
    case StatusInit::NOT_LOADED:                 return "NOT_LOADED";
    default:                                     return "N/A";
    }
}


enum class Protocol{
    TCP,
    UDP,
    RAW_MAC
};

QString inline protocolToString(Protocol proto) {
    switch(proto) {
    case Protocol::TCP:                     return "TCP";
    case Protocol::UDP:                     return "UDP";
    case Protocol::RAW_MAC:                 return "RAW_MAC";
    default:                                return "N/A";
    }
}

enum class LogType : quint8 {
    Init,
    InternalMessage,
    ProtocolMessage
};
#endif // LOGENUMS_H
