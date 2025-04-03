#include "TimeProvider.h"

// Получить экземпляр синглтона:
TimeProvider& TimeProvider::instance() {
    static TimeProvider timeProvider;
    return timeProvider;
}

// Получить время с точностью до миллисекунд:
int64_t TimeProvider::getMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

// Получить время с точностью до микросекунд:
int64_t TimeProvider::getMks() const {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

// Получить время с точностью до наносекунд:
int64_t TimeProvider::getNs() const {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

// Получить дату в текстовом виде:
std::string TimeProvider::currentDateString() {
    auto now = std::chrono::system_clock::now();
    time_t timer = std::chrono::system_clock::to_time_t(now);
    struct tm bt;
#ifdef _WIN32
    localtime_s(&bt, &timer);
#else
    localtime_r(&timer, &bt);
#endif
    char buf[32];
    strftime(buf, sizeof(buf), "%d.%m.%Y", &bt);
    return buf;
}

// Получить время в текстовом виде:
std::string TimeProvider::currentTimeString() {
    auto now = std::chrono::system_clock::now();
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(microseconds);
    microseconds -= secs;

    time_t timer = secs.count();
    struct tm bt;
#ifdef _WIN32
    localtime_s(&bt, &timer);
#else
    localtime_r(&timer, &bt);
#endif

    char buf[32];
    strftime(buf, sizeof(buf), "%H:%M:%S", &bt);
    snprintf(buf + 8, sizeof(buf) - 8, ".%06lld", static_cast<long long>(microseconds.count()));
    return buf;
}
