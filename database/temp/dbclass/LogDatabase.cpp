/*
#include "LogDatabase.h"


LogDatabase::LogDatabase(const QString& dbPath) {
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);
    if (!m_db.open()) {
        qCritical() << "DB error:" << m_db.lastError();
    }
}

bool LogDatabase::init() {
    return createInitLogTable() &&
           createInternalMessageLogTable() &&
           createProtocolMessageLogTable();
}

// Создание таблицы для InitLog
bool LogDatabase::createInitLogTable() {
    QSqlQuery query(m_db);
    return query.exec(R"(
        CREATE TABLE IF NOT EXISTS init_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp INTEGER NOT NULL,
            source TEXT NOT NULL,
            level INTEGER NOT NULL,
            status INTEGER NOT NULL
        )
    )");
}

// Создание таблицы для InternalMessageLog
bool LogDatabase::createInternalMessageLogTable() {
    QSqlQuery query(m_db);
    return query.exec(R"(
        CREATE TABLE IF NOT EXISTS internal_message_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp INTEGER NOT NULL,
            source TEXT NOT NULL,
            level INTEGER NOT NULL,
            message_type TEXT NOT NULL,
            parsed_data TEXT NOT NULL
        )
    )");
}

// Создание таблицы для ProtocolMessageLog
bool LogDatabase::createProtocolMessageLogTable() {
    QSqlQuery query(m_db);
    return query.exec(R"(
        CREATE TABLE IF NOT EXISTS protocol_message_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp INTEGER NOT NULL,
            source TEXT NOT NULL,
            level INTEGER NOT NULL,
            protocol INTEGER NOT NULL,
            raw_data BLOB,
            parsed_data TEXT
        )
    )");
}

// Вставка лога в соответствующую таблицу
bool LogDatabase::insertLog(const LogHeader& log) {
    switch (log.logType()) {
    case LogType::Init:
        return insertSpecificLog(static_cast<const InitLog&>(log));
    case LogType::InternalMessage:
        return insertSpecificLog(static_cast<const InternalMessageLog&>(log));
    case LogType::ProtocolMessage:
        return insertSpecificLog(static_cast<const ProtocolMessageLog&>(log));
    default:
        qWarning() << "Unknown log type";
        return false;
    }
}

// Шаблон для вставки конкретного типа лога
/*
template<typename T>
bool LogDatabase::insertSpecificLog(const T& log) {
    if constexpr (std::is_same_v<T, InitLog>) {
        QSqlQuery query(m_db);
        query.prepare(R"(
            INSERT INTO init_logs
                (timestamp, source, level, status)
            VALUES
                (:timestamp, :source, :level, :status)
        )");
        query.bindValue(":timestamp", log.timestampMicroseconds);
        query.bindValue(":source", log.src);
        query.bindValue(":level", static_cast<int>(log.level));
        query.bindValue(":status", static_cast<int>(log.status));
        return query.exec();
    }
    else if constexpr (std::is_same_v<T, InternalMessageLog>) {
        QSqlQuery query(m_db);
        query.prepare(R"(
            INSERT INTO internal_message_logs
                (timestamp, source, level, message_type, parsed_data)
            VALUES
                (:timestamp, :source, :level, :message_type, :parsed_data)
        )");
        query.bindValue(":timestamp", log.timestampMicroseconds);
        query.bindValue(":source", log.src);
        query.bindValue(":level", static_cast<int>(log.level));
        query.bindValue(":message_type", log.messageType);
        query.bindValue(":parsed_data", QJsonDocument(log.parsedData).toJson());
        return query.exec();
    }
    else if constexpr (std::is_same_v<T, ProtocolMessageLog>) {
        QSqlQuery query(m_db);
        query.prepare(R"(
            INSERT INTO protocol_message_logs
                (timestamp, source, level, protocol, raw_data, parsed_data)
            VALUES
                (:timestamp, :source, :level, :protocol, :raw_data, :parsed_data)
        )");
        query.bindValue(":timestamp", log.timestampMicroseconds);
        query.bindValue(":source", log.src);
        query.bindValue(":level", static_cast<int>(log.level));
        query.bindValue(":protocol", static_cast<int>(log.protocol));
        query.bindValue(":raw_data", log.rawData);
        query.bindValue(":parsed_data", QJsonDocument(log.parsedData).toJson());
        return query.exec();
    }
}
*/
/*
template<typename T>
bool LogDatabase::insertSpecificLog(const T& log) {
    if constexpr (std::is_same<T, InitLog>) {
        QSqlQuery query(m_db);
        query.prepare(R"(
            INSERT INTO init_logs
                (timestamp, source, level, status)
            VALUES
                (:timestamp, :source, :level, :status)
        )");
        query.bindValue(":timestamp", log.timestampMicroseconds);
        query.bindValue(":source", log.src);
        query.bindValue(":level", static_cast<int>(log.level));
        query.bindValue(":status", static_cast<int>(log.status));
        return query.exec();
    }
    else if constexpr (std::is_same_v<T, InternalMessageLog>) {
        QSqlQuery query(m_db);
        query.prepare(R"(
            INSERT INTO internal_message_logs
                (timestamp, source, level, message_type, parsed_data)
            VALUES
                (:timestamp, :source, :level, :message_type, :parsed_data)
        )");

        // Явное преобразование QVariantMap в QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(log.parsedData.toUtf8());
        if (jsonDoc.isNull()) {
            qWarning() << "Invalid JSON in parsedData";
            jsonDoc = QJsonDocument(); // Пустой документ при ошибке
        }


        query.bindValue(":timestamp", log.timestampMicroseconds);
        query.bindValue(":source", log.src);
        query.bindValue(":level", static_cast<int>(log.level));
        query.bindValue(":message_type", log.messageType);
        query.bindValue(":parsed_data", jsonDoc.toJson());
        return query.exec();
    }
    else if constexpr (std::is_same_v<T, ProtocolMessageLog>) {
        QSqlQuery query(m_db);
        query.prepare(R"(
            INSERT INTO protocol_message_logs
                (timestamp, source, level, protocol, raw_data, parsed_data)
            VALUES
                (:timestamp, :source, :level, :protocol, :raw_data, :parsed_data)
        )");

        // Аналогичное преобразование для ProtocolMessageLog
        QJsonDocument jsonDoc = QJsonDocument::fromJson(log.parsedData.toUtf8());
        if (jsonDoc.isNull()) {
            qWarning() << "Invalid JSON in parsedData";
            jsonDoc = QJsonDocument(); // Пустой документ при ошибке
        }

        query.bindValue(":timestamp", log.timestampMicroseconds);
        query.bindValue(":source", log.src);
        query.bindValue(":level", static_cast<int>(log.level));
        query.bindValue(":protocol", static_cast<int>(log.protocol));
        query.bindValue(":raw_data", log.rawData);
        query.bindValue(":parsed_data", jsonDoc.toJson());
        return query.exec();
    }
}


// Потоковый импорт
bool LogDatabase::importFromStream(QDataStream& stream, LogFactory& factory) {
    if (!m_db.transaction()) return false;

    while (!stream.atEnd()) {
        quint8 type;
        stream >> type;
        auto log = factory.createLog(static_cast<LogType>(type), stream);
        if (!log || !insertLog(*log)) {
            m_db.rollback();
            return false;
        }
    }



    return m_db.commit();
}

bool LogDatabase::importFromStream(QDataStream& stream, LogFactory& factory, int batchSize) {
    if (batchSize <= 0) {
        qWarning() << "Batch size must be positive";
        return false;
    }

    int totalProcessed = 0;
    bool success = true;

    while (!stream.atEnd() && success) {
        // Начало новой транзакции для пакета
        if (!m_db.transaction()) {
            qCritical() << "Failed to begin transaction";
            return false;
        }

        int currentBatch = 0;
        while (currentBatch < batchSize && !stream.atEnd()) {
            quint8 type;
            stream >> type;
            auto log = factory.createLog(static_cast<LogType>(type), stream);

            if (!log) {
                qWarning() << "Failed to parse log entry at position" << totalProcessed + currentBatch;
                continue;
            }

            if (!insertLog(*log)) {
                qWarning() << "Failed to insert log at position" << totalProcessed + currentBatch;
                m_db.rollback();
                success = false;
                break;
            }

            currentBatch++;
        }

        if (success) {
            if (!m_db.commit()) {
                qCritical() << "Failed to commit transaction";
                success = false;
            } else {
                totalProcessed += currentBatch;
                qDebug() << "Processed batch:" << currentBatch << "records. Total:" << totalProcessed;
            }
        }
    }

    return success && stream.atEnd();
}
*/
