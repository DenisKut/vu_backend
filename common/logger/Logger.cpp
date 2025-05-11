#include "Logger.h"
#include <QSqlError>
#include <QFile>
#include <qcoreapplication.h>

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
    qDebug() << "Available SQL drivers:" << QSqlDatabase::drivers();

    QSqlDatabase db;
    QString driver;
    QString errorPrefix;
    bool isWindows = false;

#ifdef Q_OS_WINDOWS
    isWindows = true;
    driver = "QODBC";
    errorPrefix = "[Windows/ODBC]";
#else
    driver = "QPSQL";
    errorPrefix = "[Linux/PSQL]";
#endif

    // Проверка доступности драйвера
    if (!QSqlDatabase::isDriverAvailable(driver)) {
        qCritical() << errorPrefix << "Driver" << driver << "not available!";
        return;
    }

    // Парсинг параметров подключения
    QMap<QString, QString> connParams;
    QStringList paramPairs;

#ifdef Q_OS_WINDOWS
    paramPairs = connectionString.split(';', Qt::SkipEmptyParts);
#else
    paramPairs = connectionString.split(' ', Qt::SkipEmptyParts);
#endif

    for (const QString& pair : paramPairs) {
        QStringList keyVal = pair.split('=', Qt::KeepEmptyParts);
        if (keyVal.size() == 2) {
            QString key = keyVal[0].trimmed().toLower();
            QString value = keyVal[1].trimmed();
            connParams[key] = value;
        }
    }

    // Валидация обязательных параметров
    const QStringList requiredParams = {"host", "dbname", "user", "password"};
    for (const QString& param : requiredParams) {
        if (!connParams.contains(param)) {
            qCritical() << errorPrefix << "Missing required parameter:" << param;
            return;
        }
    }

    // Формирование строки подключения
    QString connection;
    if (isWindows) {
        connection = QString("DRIVER=%6;"
                             "SERVER=%1;"
                             "DATABASE=%2;"
                             "UID=%3;"
                             "PWD=%4;"
                             "PORT=%5;"
                             "sslmode=disable;")
                         .arg(connParams["host"],
                              connParams["dbname"],
                              connParams["user"],
                              connParams["password"],
                              connParams.value("port", "5432"),
                              connParams["driver"]);
    } else {
        db = QSqlDatabase::addDatabase(driver);
        db.setHostName(connParams["host"]);
        db.setDatabaseName(connParams["dbname"]);
        db.setUserName(connParams["user"]);
        db.setPassword(connParams["password"]);
        db.setPort(connParams.value("port", "5432").toInt());
        db.setConnectOptions("connect_timeout=5");
    }

    // Для Windows: Настройка ODBC подключения
    if (isWindows) {
        db = QSqlDatabase::addDatabase(driver, "main_connection");
        db.setDatabaseName(connection);
    }

    qDebug() << "Connection string:" << connection;

    // Сохраняем подключение как член класса
    m_db = db;

    // Открытие подключения
    if (!m_db.open()) {
        qCritical() << errorPrefix << "Connection error:" << m_db.lastError().text();
        return;
    }

    // Убедимся, что подключение остается открытым
    m_db.setConnectOptions(""); // Убираем авто-закрытие

    // Проверка версии PostgreSQL
    QSqlQuery versionQuery(m_db);
    if (versionQuery.exec("SELECT version()") && versionQuery.next()) {
        qDebug() << "Connected to PostgreSQL:" << versionQuery.value(0).toString();
    }

    // Выполнение SQL-скрипта
    QFile sqlFile(QCoreApplication::applicationDirPath() + "/init_db.sql");
    if (!sqlFile.open(QIODevice::ReadOnly)) {
        qCritical() << errorPrefix << "Failed to open SQL file:" << sqlFile.errorString();
        return;
    }

    QStringList queries = QString(sqlFile.readAll())
                              .split(';', Qt::SkipEmptyParts);
    sqlFile.close();

    QSqlQuery query(m_db);
    bool allQueriesOK = true;

    m_db.transaction();
    for (const QString& q : queries) {
        QString trimmed = q.trimmed();
        if (trimmed.isEmpty()) continue;

        if (!query.exec(trimmed)) {
            qCritical() << errorPrefix << "Query failed:"
                        << query.lastError().text()
                        << "\nFailed query:" << trimmed;
            allQueriesOK = false;
            break;
        }
    }

    if (allQueriesOK) {
        if (!m_db.commit()) {
            qCritical() << errorPrefix << "Commit error:" << m_db.lastError().text();
            allQueriesOK = false;
        }
    } else {
        m_db.rollback();
    }

    if (allQueriesOK) {
        LOG_INIT(CLASS_NAME, StatusInit::LOADED);
        qDebug() << "Database initialized successfully";
    } else {
        qCritical() << errorPrefix << "Failed to initialize database";
    }

    start();

    // Тестовый запрос
    QSqlQuery testQuery(m_db);
    if (testQuery.exec("SELECT COUNT(*) FROM log_headers")) {
        if (testQuery.next()) {
            qDebug() << "Records in log_headers:" << testQuery.value(0).toInt();
        }
    }
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
    QMutexLocker locker(&m_mutex);
    if (m_buffer.empty() || !m_db.isOpen()) return;

    QSqlQuery query(m_db);
    m_db.transaction();

    bool success = true;
    for (const auto& q : m_buffer) {
        if (!query.exec(q)) {
            qCritical() << "Query failed:" << query.lastError().text()
            << "\nQuery:" << q;
            success = false;
            break;
        }
    }

    if (success) {
        if (!m_db.commit()) {
            qCritical() << "Commit failed:" << m_db.lastError().text();
        }
    } else {
        m_db.rollback();
    }

    m_buffer.clear();
}

void Logger::enqueueMessage(const QString& query) {
    QMutexLocker locker(&m_mutex);
    m_queryQueue.enqueue(query);
    m_condition.wakeOne();
}
