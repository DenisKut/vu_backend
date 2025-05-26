#include "Logger.h"
#include <QSqlError>
#include <QFile>
#include <qcoreapplication.h>
#include "utils/env_loader/Settings.h"
// РЕАЛИЗАЦИИ конструкторов, преобразований перед отправкой
InitLog::InitLog(
    const QString& source,
    const StatusInit& statusInit)
    : LogHeader(source, LogLevel::Info),
    status(statusInit)
{}

InitLog::InitLog(
    const QString& source,
    const LogLevel& logLevel,
    const StatusInit& statusInit)
    : LogHeader(source, logLevel),
    status(statusInit)
{}

InternalMessageLog::InternalMessageLog(
    const QString& source,
    const QString& messageType,
    const QString& messageItself)
    : LogHeader(source, LogLevel::Info),
    messageType(messageType),
    parsedData(messageItself)
{}

InternalMessageLog::InternalMessageLog(
    const QString& source,
    const LogLevel& logLevel,
    const QString& messageType,
    const QString& messageItself)
    : LogHeader(source, logLevel),
    messageType(messageType),
    parsedData(messageItself)
{}

ProtocolMessageLog::ProtocolMessageLog(
    const QString& source,
    const Protocol& protocol,
    const QByteArray& rawData,
    const QString& jsonData)
    : LogHeader(source, LogLevel::Info),
    protocol(protocol), rawData(rawData), parsedData(jsonData)
{}


ProtocolMessageLog::ProtocolMessageLog(
    const QString& source,
    const LogLevel& logLevel,
    const Protocol& protocol,
    const QByteArray& rawData,
    const QString& jsonData)
    : LogHeader(source, logLevel),
    protocol(protocol), rawData(rawData), parsedData(jsonData)
{
}

// КЛАСС ЛОГГЕРА ----------------------------------------------
// Приватный конструктор
Logger::Logger() {
    //создать файл, придать ему имя 
    //реком. sess-<human-readable-data>.bin
    //std::string curr_time_HR = TimeProvider::instance().currentTimeString();
    //открыть дескриптор записи.
    //логировать туда

}

// Приватный деструктор:
Logger::~Logger() {
    stop();
}

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::initialize(const int64_t& startTime) {
    Q_UNUSED(startTime);
    bool fileCreated = false;


    if (fileCreated)
        LOG_INIT(CLASS_NAME, StatusInit::LOADED);
    else
        LOG_INIT(CLASS_NAME, StatusInit::NOT_LOADED__ERROR_CRITICAL);  //всё равно попытаться
    start();
}

// управление очередями запросов и их отправка
void Logger::run() {
    while (!m_stop) {
        flushBuffer();
    }

    flushBuffer();
}

void Logger::stop() {
    m_stop = true;
    m_condition.wakeAll();
   
    wait();
}


void Logger::flushBuffer() {
    QMutexLocker locker(&m_mutex);
  
  
    m_buffer.clear();
}

void Logger::enqueueMessage(const QString& query) {
    QMutexLocker locker(&m_mutex);
    m_queryQueue.enqueue(query);
    m_condition.wakeOne();
}
