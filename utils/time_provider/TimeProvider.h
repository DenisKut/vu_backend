#ifndef TIMEPROVIDER_H
#define TIMEPROVIDER_H
// Класс единого времени для всех компонентов с точностью до миллисекунд (Windows/Linux)
#include <QObject>

// Для кроссплатформенности (Windows/Linux) используется библиотека <chrono>:
#include <chrono>
#include <sstream>

class TimeProvider : public QObject {
    Q_OBJECT
public:
    // Удалить копирование и присваивание:
    TimeProvider(const TimeProvider&) = delete;
    TimeProvider& operator=(const TimeProvider&) = delete;

    // Получить экземпляр синглтона:
    static TimeProvider& instance();

    // Функции получения системного времени и времени ПБУ:
    //----------------------------------------------------------------------
    // Получить время с точностью до миллисекунд:
    int64_t getMs() const;

    // Получить время с точностью до микросекунд:
    int64_t getMks() const;

    // Получить время с точностью до наносекунд:
    int64_t getNs() const;

    std::string msToDateTime(uint64_t millisecondsSinceEpoch);

    // Получить дату в текстовом виде:
    std::string currentDateString();

    // Получить время в текстовом виде:
    std::string currentTimeString();
    //----------------------------------------------------------------------

    void setPbuTimestamp1(int64_t new_time);
    void setSysTimeOnTimestamp1(int64_t new_time);
    int64_t getPbuTimeDelta();

    // Функции периода БС:
    //----------------------------------------------------------------------
    // Установить номер текущего периода БС:
    void setBsPeriodNumber(const uint32_t value);

    // Установить время текущего приода БС:
    void setBsPeriodTime(const uint8_t *value);

    // Установить значение периода синхронизации БС:
    void setBsPeriodWidth(const uint32_t value);

    // Получить номер текущего периода БС:
    uint32_t getBsPeriodNumber() const;

    // Получить время текущего приода БС:
    uint64_t getBsPeriodTime() const;

    // Получить значение периода синхронизации БС:
    uint32_t getBsPeriodWidth() const;

    void initTimes();

    // Синхронизировать время:
    void syncTime(uint32_t timeRequest1,
                  uint32_t timeRequest2,
                  uint32_t timeAnswer1,
                  uint32_t timeAnswer2);

    uint64_t getTimeForRequest();
    //----------------------------------------------------------------------


    /**
     * @brief структуры, содержащая разбор временно отметки
     * , в human-readable формат
     */
    struct HumanReadableTimestamp{
        QString data;    ///< дата в формате ГГГГ-ММ-ДД
        u_char hrs;      ///< час(ы) (0-23)
        u_char minutes;  ///< минута(ы) (0-59)
        u_char sec;      ///< секунда(ы) (0-59)
        u_short m_sec;    ///< только миллисекунды (10^-3) (0-999)
        uint mk_sec;   ///< только микросекунды (10^-6) (0-999999)
    };

    /// @brief конвертация временной отметки в структуру
    /// @param timestamp 
    /// @return структура с подробным разбором временной отметки 
    HumanReadableTimestamp to_HR_Fmt(uint64_t timestamp);

    private :
    // Приватный конструктор:
    TimeProvider() = default;

    // Переменные периода БС:
    //----------------------------------------------------------------------
    uint32_t bsPeriodNumber;        ///< Порядковый номер нового периода
    uint64_t bsPeriodTime : 40;    ///< Время начала нового периода
    uint32_t bsPeriodWidth;        ///< Значение периода синхронизации
    //----------------------------------------------------------------------

    int64_t pbuTimestamp1;          ///< Время из пакета "Отметка
                                    ///< времени" (мс)

    int64_t sysTimeOnTimestamp1;    ///< Время на момент получения
                                    ///< пакета "Отметка времени"
                                    ///< (мс)

    int64_t sysOnTimeReq;           ///< Время на момент отправки
                                    ///< пакета "Запрос времени"
                                    ///< (мс)

    int64_t pbuTimeKorr = 0;        ///< Задержка распространения
                                    ///< до ПБУ (мс)

    int64_t pbuTimeDelta = 0;       ///< Разница времени между
                                    ///< временем ПБУ и нашим
                                    ///< системным временем (мс)
};



#endif // TIMEPROVIDER_H
