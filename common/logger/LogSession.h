#ifndef LOGSESSION_H
#define LOGSESSION_H
// logsession.h
#include <QDateTime>
#include <QObject>
#include <QThread>
#include <memory>
#include "LogFactory.h"
#include "LogStorage.h"



class LogSession: public QObject {
    Q_OBJECT
public:
    explicit LogSession(const QString& filename, QObject *parent = nullptr);
    //LogSession() = default;
    ~LogSession();

    // Основной метод: приём указателя на LogHeader
    void log(LogHeader* logEntry);

    // Дополнительный метод: приём unique_ptr
    void log(std::unique_ptr<LogHeader> logEntry);

    // Шаблонный метод для удобного создания логов
    template<typename T, typename... Args>
    void log(Args&&... args) {
        log(new T(std::forward<Args>(args)...));
    }

    const QString& getSessionFileName() const { return storage.getFileName(); }



    LogStorage& getStorage() { return storage; }
    LogFactory& getFactory() { return factory; }

    void factoryInit();

signals:
    void logRequested(LogHeader* log);

public slots:
    void handleLogRequest(LogHeader* log);

    void setFileName(const QString& newFilename);

private:

    LogFactory factory;
    LogStorage storage;

    QThread workerThread;

    std::atomic<bool> shuttingDown{false};

    int LogCount = 0;
    int HandleCount = 0;
};
#endif // LOGSESSION_H
