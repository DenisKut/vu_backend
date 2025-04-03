#ifndef LOGGER_H
#define LOGGER_H
// Класс логгирования данных и всех промежуточных действий в программе (Windows/Linux)
#pragma once

#include <QObject>
#include <QDebug>
#include <QThread>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QQueue>
#include <QWaitCondition>
#include <QElapsedTimer>
#include <atomic>

#include <utils/time_provider/TimeProvider.h>

// Логгер запускается в отдельном потоке:
class Logger : public QThread
{
    Q_OBJECT
public:
    // Выполнить явную инициализацию с гарантией запуска потока:
    static void initialize();

    // Удалить копирование и присваивание:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // Получить экземпляр синглтона:
    static Logger& instance();

    // Записать текстовое сообщение в файл:
    void writeText(const QString& message);

    // Записать бинарные данные в файл в виде Hex:
    // "XX XX XX XX ..." из массива QByteArray:
    void writeBinary(const QString& message, const QByteArray& data);

    // Записать бинарные данные в файл в виде Hex:
    // "XX XX XX XX ..." из массива const unsigned char* data:
    void writeBinary(const QString& message, const unsigned char* data, unsigned long long length);

    // Запуск потока:
protected:
    void run() override;

private:
    QFile m_file;
    QMutex m_mutex;
    QWaitCondition m_condition;
    QQueue<QByteArray> m_queue;
    std::atomic<bool> m_stop{false};
    static const char* const HEX_TABLE;

    // Приватные конструктор и деструктор:
    Logger();
    ~Logger();

    // Остановка потока:
    void stop();

    // Преобразование текстовой строки с временной меткой
    // в QByteArray для дальнейшей записи в файл:
    QByteArray formatMessage(const QString& message);

    // Преобразование массива данных в Hex-формат
    // для корректного отображения в файле:
    template<typename T>
    QByteArray binaryToHex(const T* data, size_t length);

    // Преобразование QByteArray в Hex-формат
    // для корректного отображения в файле:
    QByteArray binaryToHex(const QByteArray& data);

    // Поместить сообщение в очередь на запись в лог-файл:
    void enqueueMessage(QByteArray msg);
};

#endif // LOGGER_H
