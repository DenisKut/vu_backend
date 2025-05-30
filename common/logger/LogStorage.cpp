// logstorage.cpp
#include "LogStorage.h"
#include "LogFactory.h"


LogStorage::LogStorage(const QString& filename, LogFactory& factory)
    : filename(filename), factory(factory) {}

bool LogStorage::write(const LogHeader& log) {
    QMutexLocker locker(&m_writeMutex);

    QFile file(filename);
    if (!file.open(QIODevice::Append | QIODevice::WriteOnly)) return false;

    QDataStream stream(&file);
    stream.setVersion(streamVersion);
    stream.setByteOrder(streambyteOrder);


    stream << static_cast<quint8>(log.logType()) << log;
    return stream.status() == QDataStream::Ok;;
}


std::vector<std::unique_ptr<LogHeader>> LogStorage::readAll() {
    QMutexLocker locker(&m_writeMutex);

    std::vector<std::unique_ptr<LogHeader>> logs;
    qInfo() << "hello from readall";
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) return logs;


    QDataStream stream(&file);
    stream.setVersion(streamVersion);
    stream.setByteOrder(streambyteOrder);

    while (!stream.atEnd()) {
        quint8 type;
        stream >> type;
        logs.push_back(factory.createLog(static_cast<LogType>(type), stream));
    }

    return logs;
}
