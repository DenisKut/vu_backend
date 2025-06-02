#include "LogManager.h"
#include "LogSession.h"

std::unique_ptr<LogSession> LogManager::createInstance() {
    return make_unique<LogSession>();
}

LogSession& LogManager::instance() {
    static auto session = createInstance();
    return *session;
}

void LogManager::setFileName(const QString& newFilename) {
    QMetaObject::invokeMethod(&instance(), "setFileName",
                              Qt::QueuedConnection,
                              Q_ARG(QString, newFilename));
}

QString LogManager::generateFileName() {
    return QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + "_log.bin";
}

void LogManager::setGeneratedFileName() {
    setFileName(generateFileName());
}

QString LogManager::jobj_toString(const QJsonObject& obj){
    // Compact | Indented
    return QJsonDocument(obj).toJson(QJsonDocument::Indented);
}
