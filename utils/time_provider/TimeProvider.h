#ifndef TIMEPROVIDER_H
#define TIMEPROVIDER_H
// Класс единого времени для всех компонентов с точностью до миллисекунд (Windows/Linux)
#pragma once

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

private:
    // Приватный конструктор:
    TimeProvider() = default;
};

#endif // TIMEPROVIDER_H
