#ifndef LOGMANAGER_H
#define LOGMANAGER_H
#include <memory>
#include <QJsonDocument>
#include "LogSession.h"
class LogManager {
public:
    static LogSession& instance();


    static void setFileName(const QString& newFilename);

    static QString generateFileName();

    static void setGeneratedFileName();

    static QString jobj_toString(const QJsonObject& obj);
private:
    static std::unique_ptr<LogSession> createInstance();

    LogManager() = delete;
    ~LogManager() = delete;

};
#endif // LOGMANAGER_H
