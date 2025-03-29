#include "TimeProvider.h"

TimeProvider::TimeProvider(QObject* parent) : QObject(parent) {
    m_startEpoch = QDateTime::currentMSecsSinceEpoch();
    m_timer.start();
}

quint64 TimeProvider::currentTimestamp() const {
    QMutexLocker locker(&m_mutex);
    return m_startEpoch + m_timer.elapsed() + m_timeCorrection;
}

void TimeProvider::syncWithExternalTime(quint64 externalTimeMs, quint64 networkDelayMs) {
    QMutexLocker locker(&m_mutex);
    // Текущее время приложения ДО коррекции
    quint64 currentBeforeSync = m_startEpoch + m_timer.elapsed() + m_timeCorrection;
    // Вычисляем разницу с учётом задержки сети
    m_timeCorrection = (externalTimeMs + networkDelayMs) - currentBeforeSync;
}
