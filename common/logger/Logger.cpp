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
    qDebug() << "Available SQL drivers:" << QSqlDatabase::drivers();

    QSqlDatabase db;
    QString driver;
    QString errorPrefix;

#ifdef Q_OS_WINDOWS
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


    db = QSqlDatabase::addDatabase(driver);
    {
        QVariantMap connParams = getGroupSettings("database");
        QString host           = connParams.value("host").toString().trimmed();
        QString dbname         = connParams.value("dbname").toString().trimmed();
        QString user           = connParams.value("user").toString().trimmed();
        QString password       = connParams.value("password").toString().trimmed();
        int port               = connParams.value("port").toString().trimmed().toInt();

        if (host.isEmpty())      host     = "127.0.0.1";
        if (dbname.isEmpty())    dbname   = "vu_backend";
        if (user.isEmpty())      user     = "postgres";
        if (password.isEmpty())  password = "password";
        if (port == 0)           port     = 5432;
//строки подключения
#ifdef Q_OS_WINDOWS 
        QString connection;
        if (!connParams.value("driver").toString().trimmed().isEmpty())
            driver = connParams.value("driver").toString().trimmed();
        connection = QString("DRIVER=%6;"
                        "SERVER=%1;"
                        "DATABASE=%3;"
                        "UID=%4;"
                        "PWD=%5;"
                        "PORT=%2;"
                        "sslmode=disable;")
                    .arg(host, port, dbname, user, password, driver);
#ifdef QT_DEBUG
        qDebug() << "Connection string:" << connection;
#endif
        db.setDatabaseName(connection);
#else

        db.setHostName(host);
        db.setDatabaseName(dbname);
        db.setUserName(user);
        db.setPassword(password);
        db.setPort(port);
        db.setConnectOptions("connect_timeout=5");
#endif
    }
    // Сохраняем подключение как член класса
    m_db = db;

    // Открытие подключения
    if (!m_db.open()) {
        qCritical() << "\n\n" << errorPrefix << "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
                    << "!  Connection error:\n" << m_db.lastError().text() << "\n"
                    << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
        throw std::runtime_error("Не смог подключиться. Не могу работать с БД\n\n");
        
    }

    // Убедимся, что подключение остается открытым
    m_db.setConnectOptions(""); // Убираем авто-закрытие

    // Проверка версии PostgreSQL
    QSqlQuery versionQuery(m_db);
    if (versionQuery.exec("SELECT version()") && versionQuery.next()) {
        qDebug() << "Connected to " << versionQuery.value(0).toString()<<"\n";
    }

    // Выполнение SQL-скрипта
    QFile sqlFile(QCoreApplication::applicationDirPath() + "/init_db.sql");
    if (!sqlFile.open(QIODevice::ReadOnly)) {
        qCritical() << errorPrefix << "Failed to open SQL file:" << sqlFile.errorString();
        return;
    }

    QString sql = sqlFile.readAll();
    QStringList queries;
    {
        bool inBlock = false;
        QString buffer;

        for (int i = 0; i < sql.size(); ++i) {
            if (sql.mid(i, 2) == "$$") {      // Обнаружение границ $$
                inBlock = !inBlock;
                buffer += sql[i++];           // Добавить оба символа $$
                buffer += sql[i];
                continue;
            }
            if (sql[i] == ';' && !inBlock) {  // Разделяет только вне блока $$
                if (!buffer.trimmed().isEmpty()) 
                    queries << buffer.trimmed();
                buffer.clear();
            } else {
                buffer += sql[i];
            }
        }
        if (!buffer.trimmed().isEmpty()) 
            queries << buffer.trimmed();      // Добавляет последний запрос
    }


    sqlFile.close();

    QSqlQuery query(m_db);
    bool allQueriesOK = true;

    m_db.transaction();
    for (const QString& q : queries) {
        QString trimmed = q.trimmed();
        if (trimmed.isEmpty()) continue;

        if (!query.exec(trimmed)) {
            qCritical() << errorPrefix << "ERROR occured:\n\t"
                        << query.lastError().text()
                        << "\nwhile processing query:\n\t" << trimmed;
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
        qDebug() << "Database initialized successfully";
    } else {
        qCritical() <<"\n\n"<< errorPrefix 
        << "\n !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
        <<"!\tFailed to initialize database\n"
        << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
        throw std::runtime_error("Один или несколько запросов не сработал(и). Не могу работать с БД\n\n");
    }

    //регистрация новой сессии. Без id сессии логирование запрещено
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(startTime, Qt::UTC);
    QString dtStr = dt.toString(Qt::ISODate);
    query.exec(QString("SELECT create_session('%1')").arg(dtStr));

    if (query.next()) {
        sessionID = query.value(0).toInt();
    qInfo().noquote() << 
    "╔══════════════════════════════════════════════════════╗\n"
    + QString("║ Создана сессия № %1").arg(sessionID).leftJustified(55) + "║\n"
    "╚══════════════════════════════════════════════════════╝";

    }

    if (!sessionID){
        throw std::runtime_error("Сессия не создана. Не могу работать с БД\n\n");
    }

    if (allQueriesOK)
        LOG_INIT(CLASS_NAME, StatusInit::LOADED);
    else
        LOG_INIT(CLASS_NAME, StatusInit::NOT_LOADED__ERROR_CRITICAL);  //всё равно попытаться
    start();


}

// управление очередями запросов и их отправка
void Logger::run() {
    while (!m_stop) {
        m_mutex.lock();
        if (m_queryQueue.isEmpty() && !m_stop) {
            m_condition.wait(&m_mutex);
        }

        if (!m_queryQueue.isEmpty()) {
            QString query = m_queryQueue.dequeue();
            m_buffer.push_back(query);
        }
        m_mutex.unlock();

        if (m_buffer.size() >= static_cast<std::vector<QString>::size_type>(m_bufferSize)) {
            flushBuffer();
        }

        #if defined(DB_PRINT)
            flushBuffer();
        #endif
    }

    flushBuffer();
    QSqlQuery query(m_db);
    query.prepare("UPDATE sessions SET end_time = NOW() WHERE id = :id");
    query.bindValue(":id", sessionID);
    if (!query.exec()) {
        qWarning() << "Failed to update session end_time:" << query.lastError().text();
    }
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
            qCritical() << "Query failed:" << query.lastError().text();
#ifdef DB_PRINT
            qCritical() << "\nQuery:" << q;
#endif
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
