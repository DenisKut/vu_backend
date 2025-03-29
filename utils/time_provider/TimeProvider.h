#ifndef TIMEPROVIDER_H
#define TIMEPROVIDER_H
// Класс единого времени для всех компонентов с точностью до миллисекунд (Windows/Linux)
#pragma once

#include <QObject>
#include <QElapsedTimer>
#include <QDateTime>
#include <QMutex>

class TimeProvider : public QObject {
    Q_OBJECT
public:
    explicit TimeProvider(QObject* parent = nullptr);

    // Текущее время приложения (уже корректенное)
    quint64 currentTimestamp() const;

    // Синхронизация с внешним источником времени (предположительно БС)
    void syncWithExternalTime(quint64 externalTimeMs, quint64 networkDelayMs = 0);

private:
    mutable QMutex m_mutex; // Для потокобезопасности
    QElapsedTimer m_timer;
    qint64 m_startEpoch; // Начальная эпоха (локальное время старта)
    qint64 m_timeCorrection = 0; // Коррекция времени (мс)
};

#endif // TIMEPROVIDER_H
