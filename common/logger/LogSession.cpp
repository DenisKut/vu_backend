// logsession.cpp
#include "LogSession.h"

#include "messageLogs/InternalMessageLog.h"
#include "messageLogs/ProtocolMessageLog.h"
#include "messageLogs/InitLog.h"

LogSession::LogSession(QObject *parent) : QObject(parent),
    storage(generateFileName(), factory),
    shuttingDown(false)
{
    factoryInit();
    this->moveToThread(&workerThread);

    connect(this, &LogSession::logRequested,
            this, &LogSession::handleLogRequest,
            Qt::QueuedConnection);

    workerThread.start();

    // Инициализация фабрики после перемещения в поток
    // QMetaObject::invokeMethod(this, &LogSession::factoryInit, Qt::QueuedConnection);
}

LogSession::~LogSession() {
    /*
    workerThread.quit();
    workerThread.wait();
    */
    /*
    shuttingDown = true;
    QCoreApplication::sendPostedEvents(this, QEvent::MetaCall);
    workerThread.quit();
    workerThread.wait(500); // Таймаут 500 мс
    */

    shuttingDown.store(true, std::memory_order_release);
    QCoreApplication::sendPostedEvents(this, QEvent::MetaCall);
    workerThread.quit();
    if (!workerThread.wait(500)) {
        workerThread.terminate();
        workerThread.wait();
    }
}

void LogSession::log(LogHeader* logEntry) {
    qInfo() << "HELLO FROM LOG";
    LogCount++;
    qInfo() << "LogCount = " << LogCount;
    qInfo() << "HandleCount = " << HandleCount;
    if(logEntry == nullptr) return;
    qInfo() << "LOG NUT NULL";
    logEntry->info();
    if (!shuttingDown.load(std::memory_order_acquire))
        emit logRequested(logEntry);
    else
        delete logEntry;

}

void LogSession::log(std::unique_ptr<LogHeader> logEntry) {
    qInfo() << "HELLO FROM LOG";
    LogCount++;
    qInfo() << "LogCount = " << LogCount;
    qInfo() << "HandleCount = " << HandleCount;
    if(logEntry == nullptr) return;
    qInfo() << "LOG NUT NULL";
    logEntry->info();
    if (!shuttingDown.load(std::memory_order_acquire))
        emit logRequested(logEntry.release());
}


void LogSession::handleLogRequest(LogHeader* log) {
    if(log == nullptr) return;
    std::unique_ptr<LogHeader> logPtr(log);
    if (!shuttingDown.load(std::memory_order_acquire))
        storage.write(*logPtr);
    HandleCount++;
}

QString LogSession::generateFileName() const {
    return QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + "_log.bin";
}

void LogSession::factoryInit()
{
    factory.registerType<InitLog>(LogType::Init);
    factory.registerType<InternalMessageLog>(LogType::InternalMessage);
    factory.registerType<ProtocolMessageLog>(LogType::ProtocolMessage);
}
