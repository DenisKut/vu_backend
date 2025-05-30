#ifndef LOGMANAGER_H
#define LOGMANAGER_H
#include "LogSession.h"
#include <memory>
#include "QJsonDocument"
class LogManager {
public:
    static LogSession& instance();
    static QString jobj_toString(const QJsonObject& obj){
        // Compact | Indented
        return QJsonDocument(obj).toJson(QJsonDocument::Indented);
    }
private:
    static std::unique_ptr<LogSession> createInstance();

    LogManager() = delete;
    ~LogManager() = delete;

};
#endif // LOGMANAGER_H
