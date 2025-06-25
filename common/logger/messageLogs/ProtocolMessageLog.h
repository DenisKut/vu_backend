#ifndef PROTOCOLMESSAGELOG_H
#define PROTOCOLMESSAGELOG_H
#include <QDataStream>
#include "LogHeader.h"

/**
 * @struct ProtocolMessageLog
 * @brief Лог протокольных сообщений. кидаемых вовне / принимаемых извне
 */
struct ProtocolMessageLog : public LogHeader {
    Protocol protocol;     ///< Тип сообщения/протокола
    QByteArray rawData;    ///< Сырые данные сообщения
    QString parsedData;    ///< Распарсенные данные (может быть пустой)(может быть удалена)


    /**
     * @brief констурктор 
     * @param source - источник отправляемого сообщения
     * @param protocol - тип сообщения/протокола (важный параметр, не перепутайте тип)
     * @param data - сырые данные в формате QBinaryData
     * @param jsonData - данные в формате json
     */
    ProtocolMessageLog(const QString& source,
                       const Protocol protocol,
                       const QByteArray& data,
                       const QString& jsonData);

    /**
     * @brief констурктор 
     * @param source - источник отправляемого сообщения
     * @param logLevel - уровень логирования
     * @param protocol - тип сообщения/протокола (важный параметр, не перепутайте тип)
     * @param data - сырые данные в формате QBinaryData
     * @param jsonData - данные в формате json
     */
    ProtocolMessageLog(const QString& source,
                       const LogLevel& logLevel,
                       const Protocol& protocol,
                       const QByteArray& data,
                       const QString& jsonData);

   
    /**
     * @brief констурктор 
     * @param header - заготовленный заголовок сообщения
     * @param protocol - тип сообщения/протокола (важный параметр, не перепутайте тип)
     * @param data - сырые данные в формате QBinaryData
     * @param jsonData - данные в формате json
     */
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
