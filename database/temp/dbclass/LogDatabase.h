#ifndef LOGDATABASE_H
#define LOGDATABASE_H
/*
#include <QSqlDatabase>
#include <QSqlQuery>
#include <memory>
#include "LogHeader.h"
#include "LogFactory.h"
#include <QSqlError>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include "InitLog.h"
#include "InternalMessageLog.h"
#include "ProtocolMessageLog.h"
class LogDatabase {
public:
    LogDatabase(const QString& dbPath);
    ~LogDatabase() = default;

    bool init(); // Создание всех таблиц
    bool insertLog(const LogHeader& log); // Вставка в соответствующую таблицу

    // Методы для потокового импорта
    bool importFromStream(QDataStream& stream, LogFactory& factory);
    bool importFromStream(QDataStream& stream, LogFactory& factory, int batchSize);

private:
    bool createInitLogTable();
    bool createInternalMessageLogTable();
    bool createProtocolMessageLogTable();

    template<typename T>
    bool insertSpecificLog(const T& log);

    QSqlDatabase m_db;
};
*/
#endif // LOGDATABASE_H
