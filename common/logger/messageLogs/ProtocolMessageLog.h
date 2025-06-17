#ifndef PROTOCOLMESSAGELOG_H
#define PROTOCOLMESSAGELOG_H
#include <QDataStream>
#include "LogHeader.h"

struct ProtocolMessageLog : public LogHeader {
    Protocol protocol;
    QByteArray rawData;
    QString parsedData;

    ProtocolMessageLog(const QString& source,
                       const Protocol protocol,
                       const QByteArray& data,
                       const QString& jsonData);

    ProtocolMessageLog(const QString& source,
                       const LogLevel& logLevel,
                       const Protocol& protocol,
                       const QByteArray& data,
                       const QString& jsonData);

    //
    ProtocolMessageLog(const LogHeader& header,
                       Protocol proto,
                       const QByteArray& data,
                       const QString& jsonData);

    ProtocolMessageLog() : LogHeader("", LogLevel::MessageReceived), protocol(Protocol::TCP_BINARY) {}

    LogType logType() const override { return LogType::ProtocolMessage; }

    void serialize(QDataStream& stream) const override;
    void deserialize(QDataStream& stream) override;

    void info() override
    {

        qInfo() << "PROTOCOL LOG: PARSED " << parsedData;
        qInfo() << "PROTOCOL LOG: protocol " << protocolToString(protocol);

    };
};

QDataStream& operator<<(QDataStream& stream, const ProtocolMessageLog& log);
QDataStream& operator>>(QDataStream& stream, ProtocolMessageLog& log);

#endif // PROTOCOLMESSAGELOG_H
