/**
 * @file Logger.h
 * @brief Класс для логгирования данных и действий в программе (Windows/Linux).
 * 
 * Этот файл содержит описание класса Logger, который реализует функциональность
 * логгирования текстовых и бинарных данных, а также записи логов в базу данных.
 * Логгер работает в отдельном потоке и поддерживает многопоточную запись.
 *
 * @authors Денис О, Денис Т., Никита О.
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
#include <QQueue>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDateTime>
#include <QString>
#include <atomic>
#include <vector>

#include <utils/time_provider/TimeProvider.h>


// FLAGS AND MARKS
//-------------------------------------------------------------------
enum class LogLevel {
    Trace,    ///> @brief детальная отладочная информация
	MessageReceived, ///> @brief получаемое сообщений
	MessageSent,      ///> @brief принимаемое сообщение
    Debug,    ///> @brief отладочные сообщения
    Info,     ///> @brief общая информация
    Warning,  ///> @brief предупреждения
    Error,    ///> @brief ошибки
    Fatal     ///> @brief критические ошибки
};

QString inline logLevelToString(LogLevel level) {
    switch(level) {
    case LogLevel::Trace:           return "Trace";
    case LogLevel::MessageReceived: return "MessageReceived";
    case LogLevel::MessageSent:     return "MessageSent";
    case LogLevel::Debug:           return "Debug";
    case LogLevel::Info:            return "Info";
    case LogLevel::Warning:         return "Warning";
    case LogLevel::Error:           return "Error";
    case LogLevel::Fatal:           return "Fatal";
    default:                        return "N/A";
    }
}

enum class StatusInit{
	LOADED__NO_ERRORS,
	LOADED__HAS_WARNINGS,
	NOT_LOADED__ERROR_CRITICAL,
	NOT_LOADED

};
QString inline statusInitToString(StatusInit status) {
    switch(status) {
    case StatusInit::LOADED__NO_ERRORS:       return "LOADED_NO_ERRORS";
    case StatusInit::LOADED__HAS_WARNINGS:    return "LOADED_HAS_WARNINGS";
    case StatusInit::NOT_LOADED__ERROR_CRITICAL: return "NOT_LOADED_ERROR_CRITICAL";
    case StatusInit::NOT_LOADED:              return "NOT_LOADED";
    default:                                  return "N/A";
    }
}


/**
 * @struct LogHeader
 * @brief Структура заголовка лога.
 * 
 * Содержит информацию о временной метке, источнике и получателе сообщения.
 */
struct LogHeader {
    qint64 timestampMicroseconds; ///< Время в микросекундах.
    QString src;                  ///< Источник сообщения.
    qint64 timestampMilliseconds; ///< Время в миллисекундах. Уникальный признак сообщения, сродни ID
    LogLevel level;              ///< Уровень важности

    /**
     * @brief Конструктор заголовка
     * @param source Источник
     * @param destination Получатель
     * @param logLevel Уровень логирования
     */
    LogHeader(const QString& source,
             const LogLevel& logLevel)
            : timestampMicroseconds(TimeProvider::instance().getMks()),
            src(source),
            timestampMilliseconds(TimeProvider::instance().getMs()),
            level(logLevel)
    {}
};

// END FLAGS AND MARKS ---------------------------------------------------------

// DBWRITERS основые структуры записи определенных данных в базу данных
//------------------------------------------------------------------------------
/**
 * @struct InitLog
 * @brief Лог инициализации приложения/компонентов
 */
struct InitLog : public LogHeader {
    
    StatusInit status;        ///< Сообщение инициализации
    
    InitLog(const QString& src, const StatusInit& status);
};

/**
 * @struct InternalMessageLog
 * @brief Лог внутренних сообщений между компонентами
 */
struct InternalMessageLog : public LogHeader {
    QString messageType;    ///< Тип сообщения
    QString messageItself; ///< Содержимое сообщения

	
    InternalMessageLog(const QString& src,
                       const QString& messageType,
                       const QString& messageItself);

};

/**
 * @struct ProtocolMessageLog
 * @brief Лог протокольных сообщений
 */
struct ProtocolMessageLog : public LogHeader {
    QString protocolType;   ///< Тип сообщения/протокола (TCP/UDP/RAW(MAC))
    QByteArray rawData;     ///< Сырые данные сообщения
    QString parsedData;     ///< Распарсенные данные
    
    ProtocolMessageLog(const QString& src, const QString& dst,
                      const QString& func, int line,
                      const QString& protocol, 
                      const QByteArray& data, 
                      const QString& parsed);
};

/**
 * @struct MethodCallLog
 * @brief Лог вызовов методов
 */
struct MethodCallLog : public LogHeader {
	QString callerClassName;///< Имя класса, в котором вызван метод
    QString methodName;     ///< Имя вызванного метода
    QString parameters;     ///< Параметры вызова?
    QString returnValue;    ///< Возвращаемое значение
    
    MethodCallLog(const QString& src, const QString& dst,
                 const QString& func, int line,
                 const QString& method, 
                 const QString& params = "",
                 const QString& retVal = "");

};



// END DBWRITERS ---------------------------------------------------------------
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

	Q_OBJECT

public:
	/**
	 * @brief Получить экземпляр синглтона Logger.
	 * @return Ссылка на экземпляр Logger.
	 */
	static Logger& instance();
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
	 * @param data Бинарные данные в формате QByteArray.
	 */
    // void writeBinary(const QString& message, const QByteArray& data);

	/**
	 * @brief Записать бинарные данные в файл в виде Hex.
	 * @param message Текст сообщения.
	 * @param data Бинарные данные в формате const unsigned char*.
	 * @param length Длина массива данных.
	 */
	//void writeBinary(const QString& message, const unsigned char* data, unsigned long long length);

	/**
     * @brief Основной метод логирования
     * @tparam T Тип структуры лога (InitLog, InternalMessageLog и т.д.)
     * @param logData Данные для логирования
     */
    template<typename T>
    void log(const T& logData);


protected:
	/**
	 * @brief Запуск потока логгера.
	 */
	void run() override;
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
	 * @brief Поместить SQL-запрос в очередь.
	 * @param query SQL-запрос.
	 */
	void enqueueMessage(const QString& query);

    /**
     * @brief Пакетная вставка сообщений в БД.
     * @details Все сообщения из буффера, макс m_bufferSize, закидываются в БД пачкой
    */
    void flushBuffer();

    QString bakeQuery(const LogHeader& logHead);
    QString bakeQuery(const InitLog& logData);
    QString bakeQuery(const InternalMessageLog& logData);
    //QString bakeQuery(const ProtocolMessageLog& logData);
    //QString bakeQuery(const MethodCallLog& logData);








    QSqlDatabase m_db; ///< Подключение к базе данных.
    QMutex m_mutex; ///< Мьютекс для синхронизации.
    QWaitCondition m_condition; ///< Условие ожидания.
    QQueue<QString> m_queryQueue; ///< Очередь SQL-запросов.
    std::atomic<bool> m_stop{false}; ///< Флаг остановки потока.

    std::vector<QString> m_buffer; ///< Буфер для пакетной вставки.
    const int m_bufferSize = 100; ///< Размер буфера перед записью в БД.
};
#endif // LOGGER_H
