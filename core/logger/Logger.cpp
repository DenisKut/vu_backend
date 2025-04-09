#include "Logger.h"

// Приватный конструктор
Logger::Logger() {
//    qDebug() << "Вызван конструктор лога";
}

// Приватный деструктор:
Logger::~Logger() {
    stop();
}

// Выполнить явную инициализацию с гарантией запуска потока:
void Logger::initialize() {
    auto& logger = instance();
    if (!logger.isRunning()) {
        logger.start();
        QThread::msleep(10); // Подождать 10 мс для гарантированного запуск потока
    }
}

// Получить экземпляр синглтона:
Logger& Logger::instance() {
    static Logger logger; // Экземпляр синглтона гарантированно создаётся один раз
    return logger;
}

// Записать текстовое сообщение в файл:
void Logger::writeText(const QString& message) {
    if (!isRunning()) {
        qWarning() << "ОШИБКА: поток Логгера не запустился";
        return;
    }
    enqueueMessage(formatMessage(message));
}

// Записать бинарные данные в файл в виде Hex:
// "XX XX XX XX ..." из массива QByteArray:
void Logger::writeBinary(const QString& message, const QByteArray& data) {
    enqueueMessage(formatMessage(message + ":\n") + binaryToHex(data) + "\n");
}

// Записать бинарные данные в файл в виде Hex:
// "XX XX XX XX ..." из массива const unsigned char* data:
void Logger::writeBinary(const QString& message, const unsigned char* data, unsigned long long length) {
    enqueueMessage(formatMessage(message + ":\n") + binaryToHex(data, length) + "\n");
}

// Запуск потока:
void Logger::run() {
//    qDebug() << "[Log] Thread ID:" << QThread::currentThreadId();

    QFile file("vu_backend_log.log");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qCritical() << "[Log] File error:" << file.errorString();
        return;
    }

    // Принудительная запись заголовка:
    file.write(TimeProvider::instance().currentDateString().c_str());
    file.write("\n\n");

    QElapsedTimer timer;
    timer.start();

    while (!m_stop) {
        m_mutex.lock();
        if (m_queue.isEmpty() && !m_stop) {
            m_condition.wait(&m_mutex, 100); // Таймаут 100 мс
        }
        auto temp = std::move(m_queue);
        m_mutex.unlock();

        while (!temp.isEmpty()) {
            const auto& msg = temp.dequeue();
            file.write(msg);
//            qDebug() << "[Log] Written:" << msg.size() << "bytes";
        }

        file.flush(); // Принудительный сброс буфера
    }

    file.close();
}

// Остановка Логгера:
void Logger::stop() {
    m_stop = true;
    m_condition.wakeAll();
    wait();
}

// Преобразование текстовой строки с временной меткой
// в QByteArray для дальнейшей записи в файл:
QByteArray Logger::formatMessage(const QString& message) {
    return QString("%1 - %2\n")
        .arg(TimeProvider::instance().currentTimeString().c_str())
        .arg(message)
        .toUtf8();
}

// Преобразование массива данных в Hex-формат
// для корректного отображения в файле:
template<typename T>
QByteArray Logger::binaryToHex(const T* data, size_t length) {
    QByteArray hex;
    if(length == 0) return hex;

    hex.resize(length * 3 - 1);  // Точно рассчитанный размер
    char* dest = hex.data();

    for(size_t i = 0; i < length; ++i) {
        const uint8_t c = static_cast<uint8_t>(data[i]);
        dest[0] = HEX_TABLE[c >> 4];
        dest[1] = HEX_TABLE[c & 0x0F];
        dest += 2;

        if(i != length - 1) {
            *dest++ = ' ';
        }
    }
    return hex;
}

// Явное инстанцирование для поддерживаемых типов:
template QByteArray Logger::binaryToHex<uint8_t>(const uint8_t*, size_t);
template QByteArray Logger::binaryToHex<char>(const char*, size_t);

// Преобразование QByteArray в Hex-формат
// для корректного отображения в файле:
QByteArray Logger::binaryToHex(const QByteArray& data) {
    if(data.isEmpty()) return QByteArray();
    return binaryToHex(reinterpret_cast<const uint8_t*>(data.constData()), data.size());
}

// Поместить сообщение в очередь на запись в лог-файл:
void Logger::enqueueMessage(QByteArray msg) {
    QMutexLocker lock(&m_mutex);
//    qDebug() << "[Log] Enqueuing message, size:" << msg.size();
    m_queue.enqueue(std::move(msg));
    m_condition.wakeOne();
}

const char* const Logger::HEX_TABLE = "0123456789ABCDEF";
