#ifndef LOGHEADER_H
#define LOGHEADER_H
#include <QString>
#include <QDataStream>
#include <QDebug>
#include "LogEnums.h"

struct LogHeader {
    qint64 timestampMicroseconds;
    QString src;
    LogLevel level;

    LogHeader(const QString& source, const LogLevel& logLevel);
    LogHeader(qint64 timestamp, const QString& source, LogLevel logLevel);

    virtual ~LogHeader() = default;
    virtual LogType logType() const = 0;
    virtual void info() = 0;

    virtual void serialize(QDataStream& stream) const;
    virtual void deserialize(QDataStream& stream);

};

QDataStream& operator<<(QDataStream& stream, const LogHeader& header);
QDataStream& operator>>(QDataStream& stream, LogHeader& header);


#endif // LOGHEADER_H
