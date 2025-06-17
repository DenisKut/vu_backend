#include "LogManager.h"
#include "LogSession.h"

std::unique_ptr<LogSession> LogManager::createInstance(const QString& filename) {
    return make_unique<LogSession>(filename);
}

std::unique_ptr<LogSession> LogManager::createInstance() {
    return createInstance(LogUtils::generateLogFileName());
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


void LogManager::setGeneratedFileName() {
    setFileName(LogUtils::generateLogFileName());
}

QString LogManager::jobj_toString(const QJsonObject& obj){
    // Compact | Indented
    return QJsonDocument(obj).toJson(QJsonDocument::Indented);
}
