#include "ProtocolMessageLog.h"

ProtocolMessageLog::ProtocolMessageLog(const QString& source,
                                       const Protocol protocol,
                                       const QByteArray& data,
                                       const QString& jsonData)
    : LogHeader(source, LogLevel::MessageReceived),
    protocol(protocol),
    rawData(data),
    parsedData(jsonData) {}




ProtocolMessageLog::ProtocolMessageLog(const QString& source,
                   const LogLevel& logLevel,
                   const Protocol& protocol,
                   const QByteArray& data,
                   const QString& jsonData)
    : LogHeader(source, logLevel),
    protocol(protocol),
    rawData(data),
    parsedData(jsonData)
{}


//
ProtocolMessageLog::ProtocolMessageLog(const LogHeader& header,
                   Protocol proto,
                   const QByteArray& data,
                   const QString& jsonData)
    : LogHeader(header),
    protocol(proto),
    rawData(data),
    parsedData(jsonData)
{}


// Переопределение виртуальных методов
void ProtocolMessageLog::serialize(QDataStream& stream) const {
    LogHeader::serialize(stream);
    stream << static_cast<quint8>(protocol) << rawData << parsedData;
}

void ProtocolMessageLog::deserialize(QDataStream& stream) {
    quint8 proto;
    LogHeader::deserialize(stream);
    stream >> proto >> rawData >> parsedData;
    protocol = static_cast<Protocol>(proto);
}


QDataStream& operator<<(QDataStream& stream, const ProtocolMessageLog& log) {
    /*
    stream << static_cast<const LogHeader&>(log)
    << static_cast<quint8>(log.protocol)
    << log.rawData
    << log.parsedData;
    */
    log.serialize(stream);
    return stream;
}

QDataStream& operator>>(QDataStream& stream, ProtocolMessageLog& log) {
    /*
    quint8 proto;
    stream >> static_cast<LogHeader&>(log)
        >> proto
        >> log.rawData
        >> log.parsedData;
    log.protocol = static_cast<Protocol>(proto);
    */
    log.deserialize(stream);
    return stream;
}
