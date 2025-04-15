#include "Logger.h"
#include <QSqlError>
#include <QDebug>

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
    m_db = QSqlDatabase::addDatabase("QPSQL");
    m_db.setDatabaseName(connectionString);

    if (!m_db.open()) {
        qCritical() << "Failed to connect to PostgreSQL:" << m_db.lastError().text();
        return;
    }

    start();
}

void Logger::stop() {
    m_stop = true;
    m_condition.wakeAll();
    wait();
    if (m_db.isOpen()) {
        m_db.close();
    }
}

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
    }

    // Записываем оставшиеся данные в буфере
    flushBuffer();
}

void Logger::enqueueMessage(const QString& query) {
    QMutexLocker locker(&m_mutex);
    m_queryQueue.enqueue(query);
    m_condition.wakeOne();
}

void Logger::executeQuery(const QString& query) {
    QSqlQuery sqlQuery(m_db);
    if (!sqlQuery.exec(query)) {
        qCritical() << "Failed to execute query:" << sqlQuery.lastError().text();
    }
}

void Logger::flushBuffer() {
    if (m_buffer.empty()) return;

    QSqlQuery sqlQuery(m_db);
    sqlQuery.prepare("BEGIN;");
    for (const auto& query : m_buffer) {
        if (!sqlQuery.exec(query)) {
            qCritical() << "Failed to execute query in batch:" << sqlQuery.lastError().text();
        }
    }
    sqlQuery.prepare("COMMIT;");
    sqlQuery.exec();

    m_buffer.clear();
}

void Logger::logTcpMessage(const TcpHeader& message, const QString& description) {
    QByteArray serializedData = message.serialize();
    QString hexData = serializedData.toHex(' ').toUpper();

    QString query = QString(
                        "INSERT INTO tcp_logs (timestamp, description, data_hex) "
                        "VALUES ('%1', '%2', '%3')")
                        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
                        .arg(description)
                        .arg(hexData);

    enqueueMessage(query);
}

void Logger::logInitialization(const QString& message) {
    QString query = QString("INSERT INTO initialization_logs (timestamp, message) VALUES ('%1', '%2')")
                        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
                        .arg(message);
    enqueueMessage(query);
}

void Logger::logInternalMessage(const QString& src, const QString& dst, const QString& message) {
    QString query = QString("INSERT INTO internal_logs (timestamp, src, dst, message) VALUES ('%1', '%2', '%3', '%4')")
                        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
                        .arg(src)
                        .arg(dst)
                        .arg(message);
    enqueueMessage(query);
}

void Logger::logMethodInfo(const QString& src, const QString& dst, const QString& methodName, const QString& status) {
    QString query = QString("INSERT INTO method_logs (timestamp, src, dst, method_name, status) VALUES ('%1', '%2', '%3', '%4', '%5')")
                        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
                        .arg(src)
                        .arg(dst)
                        .arg(methodName)
                        .arg(status);
    enqueueMessage(query);
}
