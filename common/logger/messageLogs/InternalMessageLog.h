#ifndef INTERNALMESSAGELOG_H
#define INTERNALMESSAGELOG_H

#include <QDataStream>
#include "LogHeader.h"

/**
 * @struct InternalMessageLog
 * @brief Лог внутренних сообщений между компонентами
 * @brief или это могут быть любые подписи
 */
struct InternalMessageLog : public LogHeader {
    QString messageType;      ///< Тип сообщения
    QString parsedData;       ///< Содержимое сообщения

    /**
     * @brief конструктор внутренних сообщений
     * @param source источник сообщения
     * @param messageType тип соообщения / заголовок
     * @param parsedData расшифровка сообщения / детальная информация
     */
    InternalMessageLog(const QString& source,
                       const QString& messageType,
                       const QString& parsedData);


    /**
     * @brief конструктор внутренних сообщений
     * @param source источник сообщения
     * @param logLevel enum LogLevel - уровень логирования
     * @param messageType тип соообщения / заголовок
     * @param parsedData расшифровка сообщения / детальная информация
     */
    InternalMessageLog(const QString& source,
                       const LogLevel& logLevel,
                       const QString& messageType,
                       const QString& parsedData);


    /**
     * @brief конструктор внутренних сообщений
     * @param header подготовленный заголовок сообщения
     * @param messageType тип соообщения / заголовок
     * @param parsedData расшифровка сообщения / детальная информация
     */
    InternalMessageLog(const LogHeader& header,
                       const QString& messageType,
                       const QString& parsedData);

    InternalMessageLog() : LogHeader("", LogLevel::MessageReceived) {}


    /**
     * @brief конвертирует тип лога в строковое представление
     */
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
