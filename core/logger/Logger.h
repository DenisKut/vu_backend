/**
 * @file Logger.h
 * @brief Класс для логгирования данных и действий в программе (Windows/Linux).
 * 
 * Этот файл содержит описание класса Logger, который реализует функциональность
 * логгирования текстовых и бинарных данных, а также записи логов в базу данных.
 * Логгер работает в отдельном потоке и поддерживает многопоточную запись.
 * 
 * @author Автор
 * @date Дата
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDateTime>
#include <QString>
#include <atomic>
#include <vector>
#include "network/messages/TcpMessage.h"

/**
 * @struct LogHeader
 * @brief Структура заголовка лога.
 * 
 * Содержит информацию о временной метке, источнике и получателе сообщения.
 */
struct LogHeader {
	qint64 timestampMicroseconds; ///< Время в микросекундах.
	QString src;                  ///< Источник сообщения.
	QString dst;                  ///< Получатель сообщения.
	QString timestampMilliseconds; ///< Время в миллисекундах.

	/**
	 * @brief Конструктор структуры LogHeader.
	 * @param source Источник сообщения.
	 * @param destination Получатель сообщения.
	 */
	LogHeader(const QString& source, const QString& destination);
};

/**
 * @class Logger
 * @brief Класс для логгирования данных.
 * 
 * Реализует функциональность записи текстовых и бинарных данных в файл,
 * а также логгирования в базу данных. Работает в отдельном потоке.
 */
class Logger : public QThread
{
	Q_OBJECT

public:
	/**
	 * @brief Получить экземпляр синглтона Logger.
	 * @return Ссылка на экземпляр Logger.
	 */
	static Logger& instance();

	/**
	 * @brief Инициализация подключения к базе данных.
	 * @param connectionString Строка подключения к базе данных.
	 */
	 void initialize(const QString& connectionString);

	/**
	 * @brief Записать текстовое сообщение в файл.
	 * @param message Текст сообщения.
	 */
    // void writeText(const QString& message);

	/**
	 * @brief Записать бинарные данные в файл в виде Hex.
	 * @param message Текст сообщения.
	 * @param data Бинарные данные в формате QByteArray.
	 */
    // void writeBinary(const QString& message, const QByteArray& data);

	/**
	 * @brief Записать бинарные данные в файл в виде Hex.
	 * @param message Текст сообщения.
	 * @param data Бинарные данные в формате const unsigned char*.
	 * @param length Длина массива данных.
	 */
	void writeBinary(const QString& message, const unsigned char* data, unsigned long long length);

    /**
     * @brief Логирование TCP-сообщений.
     * @param message Сообщение, наследуемое от TcpHeader.
     * @param description Описание сообщения.
     */
    void logTcpMessage(const TcpHeader& message, const QString& description);

    /**
     * @brief Логирование сообщений инициализации.
     * @param message Сообщение для записи.
     */
    void logInitialization(const QString& message);

    /**
     * @brief Логирование внутренних сообщений.
     * @param src Источник сообщения.
     * @param dst Получатель сообщения.
     * @param message Текст сообщения.
     */
    void logInternalMessage(const QString& src, const QString& dst, const QString& message);

    /**
     * @brief Логирование информации о методах.
     * @param src Источник вызова.
     * @param dst Получатель вызова.
     * @param methodName Имя метода.
     * @param status Статус выполнения.
     */
    void logMethodInfo(const QString& src, const QString& dst, const QString& methodName, const QString& status);

protected:
	/**
	 * @brief Запуск потока логгера.
	 */
	void run() override;

private:
	/**
	 * @brief Конструктор Logger.
	 */
	Logger();

	/**
	 * @brief Деструктор Logger.
	 */
	~Logger();

	/**
	 * @brief Остановка потока логгера.
	 */
	void stop();

	/**
	 * @brief Преобразование текстового сообщения в QByteArray для записи в файл.
	 * @param message Текст сообщения.
	 * @return Преобразованное сообщение в формате QByteArray.
	 */
	QByteArray formatMessage(const QString& message);

	/**
	 * @brief Преобразование массива данных в Hex-формат.
	 * @tparam T Тип данных.
	 * @param data Указатель на массив данных.
	 * @param length Длина массива данных.
	 * @return Преобразованные данные в формате QByteArray.
	 */
	template<typename T>
	QByteArray binaryToHex(const T* data, size_t length);

	/**
	 * @brief Преобразование QByteArray в Hex-формат.
	 * @param data Данные в формате QByteArray.
	 * @return Преобразованные данные в формате QByteArray.
	 */
	QByteArray binaryToHex(const QByteArray& data);

	/**
	 * @brief Поместить сообщение в очередь на запись в лог-файл.
	 * @param msg Сообщение в формате QByteArray.
	 */
	void enqueueMessage(QByteArray msg);

	/**
	 * @brief Логгирование внутреннего сообщения.
	 * @param header Заголовок лога.
	 * @param message Текст сообщения.
	 */
	void logInternalMessage(const LogHeader& header, const QString& message);

	/**
	 * @brief Логгирование протокольного сообщения.
	 * @param header Заголовок лога.
	 * @param protocolData Данные протокола.
	 */
	void logProtocolMessage(const LogHeader& header, const QString& protocolData);

	/**
	 * @brief Логгирование информации о методе.
	 * @param header Заголовок лога.
	 * @param methodName Имя метода.
	 * @param status Статус выполнения метода.
	 */
	void logMethodInfo(const LogHeader& header, const QString& methodName, const QString& status);

	/**
	 * @brief Поместить SQL-запрос в очередь.
	 * @param query SQL-запрос.
	 */
	void enqueueMessage(const QString& query);

	/**
	 * @brief Выполнить SQL-запрос.
	 * @param query SQL-запрос.
	 */
	void executeQuery(const QString& query);

    /**
     * @brief Пакетная вставка сообщений в БД.
     */
    void flushBuffer();

    QSqlDatabase m_db; ///< Подключение к базе данных.
    QMutex m_mutex; ///< Мьютекс для синхронизации.
    QWaitCondition m_condition; ///< Условие ожидания.
    QQueue<QString> m_queryQueue; ///< Очередь SQL-запросов.
    std::atomic<bool> m_stop{false}; ///< Флаг остановки потока.

    std::vector<QString> m_buffer; ///< Буфер для пакетной вставки.
    const int m_bufferSize = 100; ///< Размер буфера перед записью в БД.
};

/// Для создания таблиц БД:
// CREATE TABLE initialization_logs (
//     id SERIAL PRIMARY KEY,
//     timestamp TIMESTAMP NOT NULL,
//     message TEXT NOT NULL
// );

// CREATE TABLE internal_logs (
//     id SERIAL PRIMARY KEY,
//     timestamp_microseconds BIGINT NOT NULL,
//     src TEXT NOT NULL,
//     dst TEXT NOT NULL,
//     timestamp_milliseconds TEXT NOT NULL,
//     message TEXT NOT NULL
// );

// CREATE TABLE protocol_logs (
//     id SERIAL PRIMARY KEY,
//     timestamp_microseconds BIGINT NOT NULL,
//     src TEXT NOT NULL,
//     dst TEXT NOT NULL,
//     timestamp_milliseconds TEXT NOT NULL,
//     protocol_data TEXT NOT NULL
// );

// CREATE TABLE method_logs (
//     id SERIAL PRIMARY KEY,
//     timestamp_microseconds BIGINT NOT NULL,
//     src TEXT NOT NULL,
//     dst TEXT NOT NULL,
//     timestamp_milliseconds TEXT NOT NULL,
//     method_name TEXT NOT NULL,
//     status TEXT NOT NULL
// );

/// Предлагаемый пример реализации:
// #include "Logger.h"

// void exampleUsage() {
//     Logger::instance().initialize("dbname=vu_backend user=postgres password=secret");

//     LogHeader header("ExampleClass", "Database");
//     Logger::instance().logInternalMessage(header, "This is an internal message.");
//     Logger::instance().logProtocolMessage(header, "Protocol data in hex format.");
//     Logger::instance().logMethodInfo(header, "exampleMethod", "Success");

//     Logger::instance().stop();
// }

#endif // LOGGER_H
