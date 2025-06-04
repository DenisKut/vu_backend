#include "InitLog.h"

InitLog::InitLog(const QString& source, const StatusInit& status)
    : LogHeader(source, LogLevel::Info), status(status)
{}

InitLog::InitLog(const QString& source, const LogLevel& logLevel, const StatusInit& status)
    : LogHeader(source, logLevel), status(status)
{}


InitLog::InitLog(const LogHeader& header, StatusInit status)
    : LogHeader(header), status(status)
{}


void InitLog::serialize(QDataStream& stream) const {
    LogHeader::serialize(stream);
    stream << static_cast<quint8>(status);
}

void InitLog::deserialize(QDataStream& stream) {
    LogHeader::deserialize(stream);
    quint8 statusVal;
    stream >> statusVal;
    status = static_cast<StatusInit>(statusVal);
}

 QDataStream& operator<<(QDataStream& stream, const InitLog& log) {
    /*
    stream << static_cast<const LogHeader&>(log)
    << static_cast<quint8>(log.status);
    */
    log.serialize(stream);
    return stream;
}

 QDataStream& operator>>(QDataStream& stream, InitLog& log) {
    /*
    quint8 status;
    stream >> static_cast<LogHeader&>(log) >> status;
    log.status = static_cast<StatusInit>(status);
    */
    log.deserialize(stream);
    return stream;
}
