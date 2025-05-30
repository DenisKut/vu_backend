#include "InternalMessageLog.h"
#include "LogHeader.h"
InternalMessageLog::InternalMessageLog(const QString& source,
                                       const QString& messageType,
                                       const QString& parsedData)
    : LogHeader(source, LogLevel::MessageReceived),
    messageType(messageType),
    parsedData(parsedData) {}



InternalMessageLog::InternalMessageLog(const QString& source,
                   const LogLevel& logLevel,
                   const QString& messageType,
                   const QString& parsedData)
    : LogHeader(source, logLevel),
    messageType(messageType),
    parsedData(parsedData)
{}

// Constructor for deserialization
InternalMessageLog::InternalMessageLog(const LogHeader& header,
                   const QString& messageType,
                   const QString& parsedData)
    : LogHeader(header),
    messageType(messageType),
    parsedData(parsedData)
{}

void InternalMessageLog::serialize(QDataStream& stream) const {
    LogHeader::serialize(stream);
    stream << messageType << parsedData;
}

void InternalMessageLog::deserialize(QDataStream& stream) {
    LogHeader::deserialize(stream);
    stream >> messageType >> parsedData;
}

QDataStream& operator<<(QDataStream& stream, const InternalMessageLog& log) {
    /*
    stream << static_cast<const LogHeader&>(log)
           << log.messageType
           << log.parsedData;
    */
    log.serialize(stream);
    return stream;
}

QDataStream& operator>>(QDataStream& stream, InternalMessageLog& log) {
    /*
    stream >> static_cast<LogHeader&>(log)
           >> log.messageType
           >> log.parsedData;
    */
    log.deserialize(stream);
    return stream;
}
