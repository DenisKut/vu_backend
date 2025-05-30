#ifndef INTERNALMESSAGELOG_H
#define INTERNALMESSAGELOG_H

#include <QDataStream>
#include "LogHeader.h"

struct InternalMessageLog : public LogHeader {
    QString messageType;
    QString parsedData;

    InternalMessageLog(const QString& source,
                       const QString& messageType,
                       const QString& parsedData);


    InternalMessageLog(const QString& source,
                       const LogLevel& logLevel,
                       const QString& messageType,
                       const QString& parsedData);

    InternalMessageLog(const LogHeader& header,
                       const QString& messageType,
                       const QString& parsedData);

    InternalMessageLog() : LogHeader("", LogLevel::MessageReceived) {}

    LogType logType() const override { return LogType::InternalMessage; }


    void serialize(QDataStream& stream) const;
    void deserialize(QDataStream& stream) ;

    void info() override
    {

        qInfo() << "INTERNAL LOG: PARSED " << parsedData;
        qInfo() << "INTERNAL LOG: MSGTYPE " << messageType;
    };


};

QDataStream& operator<<(QDataStream& stream, const InternalMessageLog& log);
QDataStream& operator>>(QDataStream& stream, InternalMessageLog& log);
#endif // INTERNALMESSAGELOG_H
