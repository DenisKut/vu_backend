#ifndef LOGMANAGER_H
#define LOGMANAGER_H
#include <memory>
#include <QJsonDocument>
#include "LogSession.h"

//генерация названия для файла лога
//если нужно записывать в другую папку
//или если нужно другое название - изменять здесь
class LogUtils
{
public:
    static QString generateLogFileName()
    {
        return QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + "_log.bin";
    }
};

class LogManager {
public:
    static LogSession& instance();

    //установка имени файла лога (для запуска новой сесссии)
    static void setFileName(const QString& newFilename);
    //установка имени файла лога
    //(для запуска новой сесссии с автоматической генераций названия)
    static void setGeneratedFileName();

    static QString jobj_toString(const QJsonObject& obj);
private:

    static std::unique_ptr<LogSession> createInstance(const QString& filename);
    static std::unique_ptr<LogSession> createInstance();

    LogManager() = delete;
    ~LogManager() = delete;

};
#endif // LOGMANAGER_H
