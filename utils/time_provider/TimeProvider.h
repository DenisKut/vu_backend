#ifndef TIMEPROVIDER_H
#define TIMEPROVIDER_H
// Класс единого времени для всех компонентов с точностью до миллисекунд (Windows/Linux)
#pragma once

#include "qdebug.h"
#include <QObject>

// Для кроссплатформенности (Windows/Linux) используется библиотека <chrono>:
#include <chrono>

#include <QMutex>
#include <sstream>


class TimeProvider : public QObject {
    Q_OBJECT
public:
    // Удаляем копирование и присваивание
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

    // Функции записи и чтения разницы во времени
    // между ВУ и ПБУ, а также задержки сообщений
    // при передачи по TCP/IP между ВУ и ПБУ:
    //----------------------------------------------------------------------
    void setSysOnTimeReq(const int64_t time);
    void setPbuTimeDelta(const int64_t time);
    void setPbuTimeKorr(const int64_t time);
    int64_t getSysOnTimeReq() const;
    int64_t getPbuTimeDelta() const;
    int64_t getPbuTimeKorr() const;
    //----------------------------------------------------------------------

    // Функции периода БС:
    //----------------------------------------------------------------------
    // Установить номер текущего периода БС:
    void setBsPeriodNumber(const uint32_t value);

    // Установить время текущего приода БС:
    void setBsPeriodTime(const uint8_t *value);

    // Установить значение периода синхронизации БС:
    void setBsPeriodWidth(const uint32_t value);

    // Установить значение разницы во времени между ВУ и БС:
    void setBsTimeDelta(const int64_t value);

    // Установить значение времени БС по приходу сообщения 11.1:
    void setBsTimePPU(const int64_t value);

    int64_t getBsTimePPU() const;

    // Получить номер текущего периода БС:
    uint32_t getBsPeriodNumber() const;

    // Получить время текущего приода БС:
    int64_t getBsPeriodTime() const;

    // Получить значение периода синхронизации БС:
    int64_t getBsPeriodWidth() const;

    // Получить значение разницы во времени между ВУ и БС:
    int64_t getBsTimeDelta() const;

    // Получить значение разницы во времени между предыдущим
    // сообщением 11.1 и текущим временем БС:
    int64_t getBsDeltaPPU() const;

    void initTimes();

    // Синхронизировать время с БС:
    void syncTimeBS();

    // Синхронизировать время с ПБУ:
    void syncTimePBU(uint32_t timeRequest1,
                     uint32_t timeRequest2,
                     uint32_t timeAnswer1,
                     uint32_t timeAnswer2);

    // Запрос времени ПБУ для отправки сообщения на ПБУ:
    uint64_t getTimeForRequestPBU();

    // Запрос времени БС для отправки сообщения на БС:
    uint64_t getTimeForRequestBS();
    //----------------------------------------------------------------------

private:
    // Приватный конструктор:
    TimeProvider() = default;

    // QMutexLocker m_mutexLocker(QMutex&);

    mutable QMutex m_mutex{QMutex::Recursive};

    // Переменные периода БС:
    //----------------------------------------------------------------------
    int32_t bsPeriodNumber = 0;     ///< Порядковый номер нового периода
    int64_t bsPeriodTime : 40;      ///< Время начала нового периода
    int32_t bsPeriodWidth = 0;      ///< Значение периода синхронизации
    int64_t bsTimeDelta = 0;        ///< Разница времени между
                                    ///< нашим системным временем (нс)
                                    ///< и временем БС (0.1 мкс)
    int64_t bsTimePPU = 0;          ///< Время приёма последнего сообщения
                                    ///< 11.1 от ППУ
    //----------------------------------------------------------------------

    int64_t sysOnTimeReq = 0;       ///< Время на момент отправки
                                    ///< пакета "Запрос времени"
                                    ///< (мс)

    int64_t pbuTimeKorr = 0;        ///< Задержка распространения
                                    ///< до ПБУ (мс)

    int64_t pbuTimeDelta = 0;       ///< Разница времени между
                                    ///< временем ПБУ и нашим
                                    ///< системным временем (мс)
};

#endif // TIMEPROVIDER_H
