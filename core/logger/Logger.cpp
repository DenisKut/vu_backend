#include "Logger.h"
#include <QSqlError>
#include <QDebug>


#define LOG_INIT(component, message) \
    Logger::instance().log(InitLog(component, "", __FUNCTION__, __LINE__, component, message))

#define LOG_INTERNAL(src, dst, type, content) \
    Logger::instance().log(InternalMessageLog(src, dst, __FUNCTION__, __LINE__, type, content))

#define LOG_PROTOCOL(src, dst, protocol, data, parsed) \
    Logger::instance().log(ProtocolMessageLog(src, dst, __FUNCTION__, __LINE__, protocol, data, parsed))

#define LOG_METHOD(src, dst, method, params, retVal) \
    Logger::instance().log(MethodCallLog(src, dst, __FUNCTION__, __LINE__, method, params, retVal))


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
        qCritical() << "Не смог подклюбчиться к PostgreSQL:\n" << m_db.lastError().text()<<"\n\n";
        return;
    }
    

    // создать структуру БД если это еще не было сделано ранее
    //

    QString logHeader_Query = "CREATE TABLE IF NOT EXISTS log_headers ( \
    id SERIAL PRIMARY KEY, \
    src TEXT NOT NULL, \
    timestamp_micro BIGINT NOT NULL, \
    timestamp_milli BIGINT NOT NULL, \
    log_level TEXT NOT NULL CHECK (log_level IN ('TRACE', 'DEBUG', 'INFO', 'WARNING', 'ERROR', 'FATAL', 'MESSAGESENT' , 'MESSAGERECEIVED')))";

    QString initLog_Query="CREATE TABLE IF NOT EXISTS init_logs ( \
    header_id INTEGER PRIMARY KEY REFERENCES log_headers(id) ON DELETE CASCADE, \
    status TEXT NOT NULL CHECK (status IN ('LOADED_NO_ERRORS', 'LOADED_HAS_WARNINGS', 'NOT_LOADED_ERROR_CRITICAL', 'NOT_LOADED')))";

    QString internalMessage_Query = "CREATE TABLE IF NOT EXISTS internal_messages ( \
    header_id INTEGER PRIMARY KEY REFERENCES log_headers(id) ON DELETE CASCADE, \
    message_type TEXT NOT NULL, \
    message_content TEXT NOT NULL)";

    QString protocolMessage_Query = "CREATE TABLE IF NOT EXISTS protocol_messages ( \
    header_id INTEGER PRIMARY KEY REFERENCES log_headers(id) ON DELETE CASCADE, \
    protocol_type TEXT NOT NULL, \
    raw_data BYTEA, \
    parsed_data TEXT)";


    QString methodCall_Query = "CREATE TABLE IF NOT EXISTS method_calls ( \
    header_id INTEGER PRIMARY KEY REFERENCES log_headers(id) ON DELETE CASCADE, \
    caller_class TEXT NOT NULL, \
    method_name TEXT NOT NULL, \
    parameters TEXT, \
    return_value TEXT)";

    //представления VIEW, функции или процедуры, возвращающие таблицы, для линковки данных

    QSqlQuery query(m_db);
    bool statusQueryExecution = query.exec(logHeader_Query);
    statusQueryExecution &= query.exec(initLog_Query);
    statusQueryExecution &= query.exec(internalMessage_Query);
    statusQueryExecution &= query.exec(protocolMessage_Query);
    statusQueryExecution &= query.exec(methodCall_Query);
    if (statusQueryExecution){
        log(InitLog(typeid(this).name(), StatusInit::LOADED__NO_ERRORS));
    }else
        qDebug()<< "CRITICAL!!! ONE OR MORE TABLES NOT CREATED";

    //ТЕСТЫ ПОДЛЕЖАТ УДАЛЕНИЮ
    log(InternalMessageLog(typeid(this).name(), "A", "B"));

    start();
}
// управление очередями запросов и их отправка
void Logger::run() {
    int64_t lastFlushTime = TimeProvider::instance().getMs();

    while (!m_stop) {
        m_mutex.lock();


        if (m_queryQueue.isEmpty() && !m_stop) {
            int64_t currentTime = TimeProvider::instance().getMs();
            int64_t timeSinceLastFlush = currentTime - lastFlushTime;
            int64_t timeToWait = std::max(0LL, 1000LL - timeSinceLastFlush);

            if (timeToWait > 0) {
                m_condition.wait(&m_mutex, static_cast<unsigned long>(timeToWait));
            }
        }

        if (!m_queryQueue.isEmpty()) {
            QString query = m_queryQueue.dequeue();
            m_buffer.push_back(query);
        }

        int64_t currentTime = TimeProvider::instance().getMs();
        bool timeToFlush = (currentTime - lastFlushTime) >= 1000;

        if (m_buffer.size() >= static_cast<std::vector<QString>::size_type>(m_bufferSize) ||
            (timeToFlush && !m_buffer.empty())) {
            flushBuffer();
            lastFlushTime = currentTime;
        }

        m_mutex.unlock();
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

        //qDebug()<<"\n\n"<<query<<"\n\n";

        if (!sqlQuery.exec(query)) {
            qCritical() << "Failed to execute query in batch:" << sqlQuery.lastError().text();
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
QString Logger::bakeQuery(const LogHeader& logHead) {
    return QString("INSERT INTO log_headers "
                   "(src, timestamp_micro, timestamp_milli, log_level) "
                   "VALUES ('%1', %2, %3, '%4') "
                   "RETURNING id")
        .arg(logHead.src)
        .arg(logHead.timestampMicroseconds)
        .arg(logHead.timestampMilliseconds)
        .arg(logLevelToString(logHead.level).toUpper());
}

//связь один-к-одному

QString Logger::bakeQuery(const InitLog& logData) {
    return QString("WITH header AS (%1) "
                   "INSERT INTO init_logs (header_id, status) "
                   "SELECT id, '%2' FROM header;")
        .arg(bakeQuery(static_cast<const LogHeader&>(logData)),
        statusInitToString(logData.status));
}



QString Logger::bakeQuery(const InternalMessageLog& logData) {
    return QString("WITH header AS (%1) "
                   "INSERT INTO internal_messages (header_id, message_type, message_content) "
                   "SELECT id, '%2', '%3' FROM header;")
        .arg(bakeQuery(static_cast<const LogHeader&>(logData)),
        logData.messageType,
        logData.messageItself);
}

/// @brief Главный логирующий универсальный публичный метод
/// @param T универсальный параметр, может быть одрной из 4х структур типа логирования
/// @details проверить, структура из списка ли, затем создать для неё запрос, запрос разместить в очередь
template<typename T>
void Logger::log(const T& logData) {
    //чекер наличия
    static_assert(
        std::is_same<T, InitLog>::value ||
        std::is_same<T, InternalMessageLog>::value ||
        std::is_same<T, ProtocolMessageLog>::value ||
        std::is_same<T, MethodCallLog>::value,
        "N/A"
        );
    //формирование запроса
    QString query = bakeQuery(logData);
    //в очередь
    enqueueMessage(query);

    m_condition.wakeOne();
}

// РЕАЛИЗАЦИИ конструкторов, преобразований перед отправкой
InitLog::InitLog(const QString& src, const StatusInit& statusInit)
    : LogHeader(src, LogLevel::Info),
    status(statusInit)
{}

InternalMessageLog::InternalMessageLog(const QString& src,
                   const QString& messageType,
                   const QString& messageItself)
    : LogHeader(src, LogLevel::MessageSent), messageType(messageType), messageItself(messageItself)
{}






// КОНЕЦ РЕАЛИЗАЦИИ



