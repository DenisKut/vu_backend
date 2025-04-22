#ifndef TIMEPROVIDER_H
#define TIMEPROVIDER_H
// Класс единого времени для всех компонентов с точностью до миллисекунд (Windows/Linux)
#pragma once

#include "qdebug.h"
#include <QObject>

// Для кроссплатформенности (Windows/Linux) используется библиотека <chrono>:
#include <chrono>

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

    // Получить дату в текстовом виде:
    std::string currentDateString();

    // Получить время в текстовом виде:
    std::string currentTimeString();
    //----------------------------------------------------------------------

    void setPbuTimestamp(int64_t new_time);
    void setTimeOnTimestamp(int64_t new_time);

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

    // Синхронизировать время:
    void syncTime(uint32_t time1, uint32_t time2);

    uint64_t getTimeForRequest();

    //----------------------------------------------------------------------

private:
    // Приватный конструктор:
    TimeProvider() = default;

    // Переменные периода БС:
    //----------------------------------------------------------------------
    uint32_t bsPeriodNumber;        ///< Порядковый номер нового периода
    uint64_t bsPeriodTime : 40;    ///< Время начала нового периода
    uint32_t bsPeriodWidth;        ///< Значение периода синхронизации
    //----------------------------------------------------------------------

    int64_t pbuTimestamp = getMs();     ///< Время из пакета "Отметка
                                        ///< времени" (мс)

    int64_t timeOnTimestamp = getMs();  ///< Время на момент получения
                                        ///< пакета "Отметка времени"
                                        ///< (мс)
};

#endif // TIMEPROVIDER_H
