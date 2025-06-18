#ifndef LOGSTORAGE_H
#define LOGSTORAGE_H
#include <QString>
#include <QFile>
#include <vector>
#include <memory>
#include <QMutex>

#include "messageLogs/LogHeader.h"
#include "LogFactory.h"
class LogStorage {
public:

    LogStorage(const QString& filename, LogFactory& factory);
    /*  шаблонный write можно в теории оставить, если нужена будет реализация без наследников
    template<typename T>
    bool write(const T& log) {
        QMutexLocker locker(&m_writeMutex);

        QFile file(filename);
        if (!file.open(QIODevice::Append | QIODevice::WriteOnly)) return false;

        QDataStream stream(&file);
        stream.setVersion(streamVersion);
        stream << static_cast<quint8>(log.logType()) << log;
        return stream.status() == QDataStream::Ok;;
    }
    */
    bool write(const LogHeader& log);

    std::vector<std::unique_ptr<LogHeader>> readAll();

    const QString& getFileName() const;

    void setFileName(const QString& newFilename);

private:
    QString filename;
    LogFactory& factory;

    QDataStream::Version streamVersion = QDataStream::Qt_5_14;
    QDataStream::ByteOrder streambyteOrder = QDataStream::LittleEndian;

    QMutex m_writeMutex;
};
#endif // LOGSTORAGE_H
