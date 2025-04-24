/**
 * @file Logger.h
 * @brief Класс для логгирования данных и действий в программе (Windows/Linux).
 * 
 * Этот файл содержит описание класса Logger, который реализует функциональность
 * логгирования текстовых и бинарных данных, а также записи логов в базу данных.
 * Логгер работает в отдельном потоке и поддерживает многопоточную запись.
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
#include <QJsonDocument>



// FLAGS AND MARKS
//-------------------------------------------------------------------

/**
 * @brief идентификатор, который принадлежит структуре LogHeader
 * @details статусы сообщений, позволяет разделять их по важности
 */
enum class LogLevel {
    //mostly debug ¯\_(ツ)_/¯
    Trace,    ///> @brief детальная отладочная информация
    Debug,    ///> @brief отладочные сообщения
    //common
    Info,     ///> @brief общая информация
    Warning,  ///> @brief предупреждения
    Error,    ///> @brief ошибки
    Fatal,     ///> @brief критические ошибки
    //message in/out
    MessageReceived, ///> @brief получаемое сообщений
    MessageSent      ///> @brief принимаемое сообщение
};

QString inline logLevelToString(LogLevel level) {
    switch(level) {
    case LogLevel::Trace:           return "Trace";
    case LogLevel::Debug:           return "Debug";
    case LogLevel::Info:            return "Info";
    case LogLevel::Warning:         return "Warning";
    case LogLevel::Error:           return "Error";
    case LogLevel::Fatal:           return "Fatal";
    case LogLevel::MessageReceived: return "MessageReceived";
    case LogLevel::MessageSent:     return "MessageSent";
    default:                        return "N/A";
    }
}

/**
 * @brief приналежит InitLog
 * @details помогает разделять сообщения об инициализации компонентов по важности
 */
enum class StatusInit{
    LOADED,
    NOT_LOADED__ERROR_CRITICAL,
    NOT_LOADED
};

QString inline statusInitToString(StatusInit status) {
    switch(status) {
    case StatusInit::LOADED:                     return "LOADED";
    case StatusInit::NOT_LOADED__ERROR_CRITICAL: return "NOT_LOADED_ERROR_CRITICAL";
    case StatusInit::NOT_LOADED:                 return "NOT_LOADED";
    default:                                     return "N/A";
    }
}


enum class Protocol{
    TCP,
    UDP,
    RAW_MAC
};

QString inline protocolToString(Protocol proto) {
    switch(proto) {
    case Protocol::TCP:                     return "TCP";
    case Protocol::UDP:                     return "UDP";
    case Protocol::RAW_MAC:                 return "RAW_MAC";
    default:                                return "N/A";
    }
}

// END FLAGS AND MARKS ---------------------------------------------------------


//--------------------------------------------------------------------------------
// DB_WRITABLE STRUCTS основые структуры записи определенных данных в базу данных
//--------------------------------------------------------------------------------
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



/**
 * @struct InitLog
 * @brief Лог инициализации приложения/компонентов
 */
struct InitLog : public LogHeader {

    StatusInit status;        ///< Сообщение инициализации

    InitLog(const QString& source,
            const StatusInit& status);

    InitLog(const QString& source,
            const LogLevel& logLevel,
            const StatusInit& status);
};

/**
 * @struct InternalMessageLog
 * @brief Лог внутренних сообщений между компонентами
 */
struct InternalMessageLog : public LogHeader {
    QString messageType;    ///< Тип сообщения
    QString parsedData; ///< Содержимое сообщения


    InternalMessageLog(const QString& source,
                       const QString& messageType,
                       const QString& parsedData);

    InternalMessageLog(const QString& source,
                       const LogLevel& logLevel,
                       const QString& messageType,
                       const QString& parsedData);
};

/**
 * @struct ProtocolMessageLog
 * @brief Лог протокольных сообщений. кидаемых вовне / принимаемых извне
 */
struct ProtocolMessageLog : public LogHeader {
    Protocol protocol;   ///< Тип сообщения/протокола (TCP/UDP/RAW(MAC))
    QByteArray rawData;     ///< Сырые данные сообщения
    QString parsedData;     ///< Распарсенные данные

    ProtocolMessageLog(const QString& source,
                       const Protocol& protocol,
                       const QByteArray& data,
                       const QString& jsonData);

    ProtocolMessageLog(const QString& source,
                       const LogLevel& logLevel,
                       const Protocol& protocol,
                       const QByteArray& data,
                       const QString& jsonData);

};
// END DB_WRITABLE STRUCTS--------------------------------------------------------------


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
	//void writeBinary(const QString& message, const unsigned char* data, unsigned long long length);


     /// @brief Главный логирующий универсальный публичный метод
     /// @param T универсальный параметр, может быть одрной из ~~4х~~ 3х структур типа логирования
     /// @details проверить, структура из списка ли, затем создать для неё запрос, запрос разместить в очередь
     template<typename T>
     void log(const T& logData) {
         static_assert(
             std::is_same<T, InitLog>::value ||
             std::is_same<T, InternalMessageLog>::value ||
             std::is_same<T, ProtocolMessageLog>::value,
             "поданая структура должна быть одной из: InitLog InternalMessageLog ProtocolMessageLog"
             );

         enqueueMessage(bakeQuery(logData));
         m_condition.wakeOne();
     }


     QString bakeQuery(const InitLog& logData) {
         return QString("WITH header AS (%1) "
                        "INSERT INTO init_logs (header_id, status) "
                        "SELECT id, '%2' FROM header;")
             .arg(bakeQuery(static_cast<const LogHeader&>(logData)),
                  statusInitToString(logData.status));
     }

     QString bakeQuery(const InternalMessageLog& logData) {
         return QString("WITH header AS (%1) "
                        "INSERT INTO internal_messages (header_id, message_type, message_content) "
                        "SELECT id, '%2', '%3' FROM header;")
             .arg(bakeQuery(static_cast<const LogHeader&>(logData)),
                  logData.messageType,
                  logData.parsedData);
     }

     QString bakeQuery(const ProtocolMessageLog& logData) {
         return QString("WITH header AS (%1) "
                        "INSERT INTO protocol_messages (header_id, protocol_type, raw_data, parsed_data) "
                        "SELECT id, '%2', '%3', '%4' FROM header;")
             .arg(bakeQuery(static_cast<const LogHeader&>(logData)),
                  protocolToString(logData.protocol),
                  logData.rawData,
                  logData.parsedData);
     }

     // CONVERTERS --------------------------------------------------------------------------
     /**
     * @brief jobj_toString функция для конвертации QJsonObject в QString
     * @param obj QJSonObject - объект, содержащий в себе описываемую структуру
     * @return QString - строка в ф-те JSON
     */
     static QString jobj_toString(const QJsonObject& obj){
         // Compact | Indented
         return QJsonDocument(obj).toJson(QJsonDocument::Indented);
     }

     // END CONVERTERS ----------------------------------------------------------------------



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


    // /**
    //  * @brief Преобразование текстового сообщения в QByteArray для записи в файл.
    //  * @param message Текст сообщения.
    //  * @return Преобразованное сообщение в формате QByteArray.
    //  */
    // QByteArray formatMessage(const QString& message);


    // /**
    //  * @brief Преобразование массива данных в Hex-формат.
    //  * @tparam T Тип данных.
    //  * @param data Указатель на массив данных.
    //  * @param length Длина массива данных.
    //  * @return Преобразованные данные в формате QByteArray.
    //  */
    // template<typename T>
    // QByteArray binaryToHex(const T* data, size_t length);


    // /**
    //  * @brief Преобразование QByteArray в Hex-формат.
    //  * @param data Данные в формате QByteArray.
    //  * @return Преобразованные данные в формате QByteArray.
    //  */
    // QByteArray binaryToHex(const QByteArray& data);

    // /**
    //  * @brief Поместить сообщение в очередь на запись в лог-файл.
    //  * @param msg Сообщение в формате QByteArray.
    //  */
    // void enqueueMessage(QByteArray msg);

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

    QString bakeQuery(const LogHeader& logHead) {
        return QString("INSERT INTO log_headers "
                       "(src, timestamp_micro, timestamp_milli, log_level) "
                       "VALUES ('%1', %2, %3, '%4') RETURNING id")
            .arg(logHead.src)
            .arg(logHead.timestampMicroseconds)
            .arg(logHead.timestampMilliseconds)
            .arg(logLevelToString(logHead.level).toUpper());
    }



    QSqlDatabase m_db; ///< Подключение к базе данных.
    QMutex m_mutex; ///< Мьютекс для синхронизации.
    QWaitCondition m_condition; ///< Условие ожидания.
    QQueue<QString> m_queryQueue; ///< Очередь SQL-запросов.
    std::atomic<bool> m_stop{false}; ///< Флаг остановки потока.

    std::vector<QString> m_buffer; ///< Буфер для пакетной вставки.
    const int m_bufferSize = 100; ///< Размер буфера перед записью в БД.
};
#endif // LOGGER_H
