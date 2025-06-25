#ifndef LOGHEADER_H
#define LOGHEADER_H
#include <QString>
#include <QDataStream>
#include <QDebug>
#include "LogEnums.h"


/**
 * @struct LogHeader
 * @brief Структура заголовка лога.
 *
 * Содержит информацию о временной метке, источнике и получателе сообщения.
 */
struct LogHeader {
    qint64 timestampMicroseconds;    ///< Время в микросекундах.
    QString src;                     ///< Источник сообщения.
    LogLevel level;                  ///< Уровень важности

    /**
     * @brief Конструктор заголовка
     * @param source Источник
     * @param logLevel Уровень логирования
     */
    LogHeader(const QString& source, const LogLevel& logLevel);
    /**
     * @brief Конструктор заголовка
     * @param timestamp отметка времени
     * @param source Источник
     * @param logLevel Уровень логирования
     */
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
