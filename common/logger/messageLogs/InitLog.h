#ifndef INITLOG_H
#define INITLOG_H
#include "LogHeader.h"

/**
 * @struct InitLog
 * @brief Лог инициализации приложения/компонентов
 */
struct InitLog : public LogHeader {
    StatusInit status;   ///< Сообщение инициализации компонента

    /**
     * @brief конструктор сообщения инициализации
     * @param source компонент - источник сообщения
     * @param status StatusInit - enum, одно из вероятных состояний компонента
     */
    InitLog(const QString& source, const StatusInit& status);
       
    /**
     * @brief конструктор сообщения инициализации
     * @param source компонент - источник сообщения
     * @param logLevel - Уровень логирования
     * @param status StatusInit - enum, одно из вероятных состояний компонента
     */
    InitLog(const QString& source, const LogLevel& logLevel, const StatusInit& status);

    /**
     * @brief конструктор сообщения инициализации
     * @param header - подготовленный заголовок LogHeader сообщения 
     * @param status StatusInit - enum, одно из вероятных состояний компонента
     */
    InitLog(const LogHeader& header, StatusInit status);


    InitLog() : LogHeader("", LogLevel::Info), status(StatusInit::NOT_LOADED) {}

    LogType logType() const override { return LogType::Init; }


    void serialize(QDataStream& stream) const override;
    void deserialize(QDataStream& stream) override;


    void info() override
    {

        qInfo() << "INIT LOG: STATUS " << statusInitToString(status);
    };


};

QDataStream& operator<<(QDataStream& stream, const InitLog& log);
QDataStream& operator>>(QDataStream& stream, InitLog& log);

#endif // INITLOG_H
