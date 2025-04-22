#include "Logger.h"
#include <QSqlError>
#include <QDebug>
#include <QFile>

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
Logger::Logger() {}

// Приватный деструктор:
Logger::~Logger() {
    stop();
}

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::initialize(const QString& connectionString) {


    qDebug() << "Драйвера:" << QSqlDatabase::drivers();

    if (!QSqlDatabase::isDriverAvailable("QPSQL")) {
        qCritical() << "QPSQL недоступен/не найден!";
        return;
    }


    m_db = QSqlDatabase::database();
    if (!m_db.isValid()) {
        m_db = QSqlDatabase::addDatabase("QPSQL", "postgres");
        m_db.setConnectOptions(connectionString+" connect_timeout=5");
    }

    if (!m_db.open()) {
        qCritical() << "Не смог подключиться к PostgreSQL:\n" << m_db.lastError().text()<<"\n\n";
        return;
    }


    // создать структуру БД если это еще не было сделано ранее
    //

    QFile sqlFile("../../init_db.sql");
    if (!sqlFile.open(QIODevice::ReadOnly)) {
        qCritical() << "\n\n=========    !!!НЕ НАЙДЕН ФАЙЛ SQL!!!    =========\n" << sqlFile.errorString() <<"\n==================================================\n\n";
        return;
    }

    QStringList queries = QString(sqlFile.readAll()).split(';', Qt::SkipEmptyParts);
    sqlFile.close();

    QSqlQuery query(m_db);
    bool statusQueryExecution = false;
    for (const QString& q : queries) {
        QString trimmed = q.trimmed();
        if (!trimmed.isEmpty() && !query.exec(trimmed)) {
            qCritical() << "\n\n=========    !!!НЕ ВЫПОЛНЕН ЗАПРОС!!!    =========\n" << query.lastError().text()<<"\n==================================================\n\n";
            qCritical() << "Провальный запрос:" << trimmed;
            statusQueryExecution = false;
            break;
        }
        statusQueryExecution = true;
    }


    if (statusQueryExecution){
        LOG_INIT(typeid(this).name(), StatusInit::LOADED);
    }else
        qCritical()<< "CRITICAL!!! ONE OR MORE TABLES NOT CREATED";

    #ifdef QT_DEBUG
        //log(InternalMessageLog(typeid(this).name(), "orig", "processed"));
        //LOG_INTERNAL(typeid(this).name(), LogLevel::Debug, "orig1", "processed1");
        //log(ProtocolMessageLog(typeid(this).name(), "UDP", "orig"));
        //LOG_PROTOCOL(typeid(this).name(), "MAC", "RAW");
    #endif
    start();
}
// управление очередями запросов и их отправка
void Logger::run() {
    while (!m_stop) {
        m_mutex.lock();
        if (m_queryQueue.isEmpty() && !m_stop) {
            m_condition.wait(&m_mutex);
        }
        QString query = m_queryQueue.dequeue();
        m_mutex.unlock();

        m_buffer.push_back(query);
        if (m_buffer.size() >= static_cast<std::vector<QString>::size_type>(m_bufferSize)) {
            flushBuffer();
        }


        #ifdef QT_DEBUG
            flushBuffer();
        #endif
    }

    flushBuffer();
}

void Logger::stop() {
    m_stop = true;
    m_condition.wakeAll();
    wait();
    if (m_db.isOpen()) {
        m_db.close();
    }
}


void Logger::flushBuffer() {
    if (m_buffer.empty()) return;

    QSqlQuery sqlQuery(m_db);
    sqlQuery.prepare("BEGIN;");
    for (const auto& query : m_buffer) {
        #ifdef QT_DEBUG
            qDebug()<<"\n"<<query;
        #endif
        if (!sqlQuery.exec(query)) {
            #ifdef QT_DEBUG
                qCritical() << "\n\n=========    !!!НЕ ВЫПОЛНЕН ЗАПРОС!!!    =========\n" << sqlQuery.lastError().text()<<"\n==================================================\n\n";
            #endif
        }
    }
    sqlQuery.prepare("COMMIT;");
    sqlQuery.exec();

    m_buffer.clear();
}

void Logger::enqueueMessage(const QString& query) {
    QMutexLocker locker(&m_mutex);
    m_queryQueue.enqueue(query);
    m_condition.wakeOne();
}

// КОНЕЦ управление очередями и отправка

QString bakeQuery(const LogHeader& logHead) {
    return QString("INSERT INTO log_headers "
                   "(src, timestamp_micro, timestamp_milli, log_level) "
                   "VALUES ('%1', %2, %3, '%4') RETURNING id")
        .arg(logHead.src)
        .arg(logHead.timestampMicroseconds)
        .arg(logHead.timestampMilliseconds)
        .arg(logLevelToString(logHead.level).toUpper());
}

QString bakeQuery(const InitLog& logData) {
    return QString("WITH header AS (%1) "
                   "INSERT INTO init_logs (header_id, status) "
                   "SELECT id, '%2' FROM header;")
        .arg(bakeQuery(static_cast<const LogHeader&>(logData)),
             statusInitToString(logData.status));
}

QString bakeQuery(const InternalMessageLog& logData) {
    return QString("WITH header AS (%1) "
                   "INSERT INTO internal_messages (header_id, message_type, message_content) "
                   "SELECT id, '%2', '%3' FROM header;")
        .arg(bakeQuery(static_cast<const LogHeader&>(logData)),
             logData.messageType,
             logData.parsedData);
}

QString bakeQuery(const ProtocolMessageLog& logData) {
    return QString("WITH header AS (%1) "
                   "INSERT INTO protocol_messages (header_id, protocol_type, raw_data, parsed_data) "
                   "SELECT id, '%2', '%3', '%4' FROM header;")
        .arg(bakeQuery(static_cast<const LogHeader&>(logData)),
             protocolToString(logData.protocol),
             logData.rawData,
             logData.parsedData);
}


/// @brief Главный логирующий универсальный публичный метод
/// @param T универсальный параметр, может быть одрной из ~~4х~~ 3х структур типа логирования
/// @details проверить, структура из списка ли, затем создать для неё запрос, запрос разместить в очередь
template<typename T>
void Logger::log(const T& logData) {
    static_assert(
        std::is_same<T, InitLog>::value ||
            std::is_same<T, InternalMessageLog>::value ||
            std::is_same<T, ProtocolMessageLog>::value,
        "поданая структура должна быть одной из: InitLog InternalMessageLog ProtocolMessageLog"
        );

    enqueueMessage(bakeQuery(logData));
    m_condition.wakeOne();
}




