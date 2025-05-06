#include "TimeProvider.h"
#include <iomanip>

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

std::string TimeProvider::msToDateTime(uint64_t millisecondsSinceEpoch)  {
    // Конвертируем миллисекунды в секунды и отдельно получаем миллисекунды
    time_t seconds = static_cast<time_t>(millisecondsSinceEpoch / 1000);
    unsigned int milliseconds = static_cast<unsigned int>(millisecondsSinceEpoch % 1000);

    // Преобразуем в struct tm (локальное время)
    struct tm timeinfo;
    localtime_r(&seconds, &timeinfo); // Используйте localtime_s для Windows

    // Форматируем в строку
    std::ostringstream oss;
    oss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S")
        << "." << std::setfill('0') << std::setw(3) << milliseconds;

    return oss.str();
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

// Установить номер текущего периода БС:
void TimeProvider::setBsPeriodNumber(const uint32_t value) {
    bsPeriodNumber = value;
}

// Установить время текущего приода БС:
void TimeProvider::setBsPeriodTime(const uint8_t *value) {
    for (int i = 4; i >= 0; --i) { // Обратный порядок (Little-endian)
        bsPeriodTime = (bsPeriodTime << 8) | value[i];
    }
    bsPeriodTime &= 0x000000FFFFFFFFFF;
}

// Установить значение периода синхронизации БС:
void TimeProvider::setBsPeriodWidth(const uint32_t value) {
    bsPeriodWidth = value;
}

// Получить номер текущего периода БС:
uint32_t TimeProvider::getBsPeriodNumber() const {
    return bsPeriodNumber;
}

// Получить время текущего приода БС:
uint64_t TimeProvider::getBsPeriodTime() const {
    return bsPeriodTime;
}

// Получить значение периода синхронизации БС:
uint32_t TimeProvider::getBsPeriodWidth() const {
    return bsPeriodWidth;
}

void TimeProvider::initTimes() {
    int64_t time_now = getMs();
    pbuTimestamp1 = time_now;
    sysTimeOnTimestamp1 = time_now;
}

void TimeProvider::setPbuTimestamp1(int64_t new_time) {
    pbuTimestamp1 = new_time;
}

void TimeProvider::setSysTimeOnTimestamp1(int64_t new_time) {
    sysTimeOnTimestamp1 = new_time;
}

int64_t TimeProvider::getPbuTimeDelta() {
    return pbuTimeDelta;
}

// Синхронизировать время:
void TimeProvider::syncTime(uint32_t timeRequest1,
                            uint32_t timeRequest2,
                            uint32_t timeAnswer1,
                            uint32_t timeAnswer2) {


    // Объединяем два 32-битных значения в 64-битное время
    int64_t timePBU =
        static_cast<int64_t>(static_cast<uint64_t>(timeAnswer2) << 32)
                                | timeAnswer1;

    int64_t timePBURequest =
        static_cast<int64_t>(static_cast<uint64_t>(timeRequest2) << 32)
        | timeRequest1;

    setPbuTimestamp1(timePBU);

    // Получаем текущее время системы
    int64_t time_now = getMs();
    setSysTimeOnTimestamp1(time_now);

    pbuTimeDelta = timePBU - time_now;
    pbuTimeKorr = timePBU - timePBURequest;
    qDebug() << "pbuTimeDelta =" << pbuTimeDelta;
    qDebug() << "pbuTimeKorr =" << pbuTimeKorr;
}

uint64_t TimeProvider::getTimeForRequest() {
    sysOnTimeReq = getMs();
    int64_t _pbuTimeDelta = getPbuTimeDelta();
    qDebug() << "sysOnTimeReq =" << sysOnTimeReq;
    qDebug() << "pbuTimeDelta =" << pbuTimeDelta;
    uint64_t resultTime = static_cast<uint64_t>(
            sysOnTimeReq + _pbuTimeDelta
       );
    return resultTime;
}

TimeProvider::HumanReadableTimestamp TimeProvider::to_HR_Fmt(uint64_t timestamp)
{
    using namespace std::chrono;
    HumanReadableTimestamp timeInfo;

    auto duration = microseconds(timestamp);
    system_clock::time_point tp(duration);

    auto tp_seconds = time_point_cast<seconds>(tp);
    auto subseconds = duration_cast<microseconds>(tp - tp_seconds);

    time_t time = system_clock::to_time_t(tp_seconds);
    std::tm tm = *std::localtime(&time);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    timeInfo.data = QString::fromStdString(oss.str());

    timeInfo.hrs = static_cast<u_char>(tm.tm_hour);
    timeInfo.minutes = static_cast<u_char>(tm.tm_min);
    timeInfo.sec = static_cast<u_char>(tm.tm_sec);
    timeInfo.m_sec = static_cast<u_short>(subseconds.count() / 1000);
    timeInfo.mk_sec = static_cast<u_short>(subseconds.count());
    return timeInfo;
}
