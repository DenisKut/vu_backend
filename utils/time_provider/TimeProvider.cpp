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
//    bsPeriodTime = from_timeBS(value);
    for (int i = 4; i >= 0; --i) { // Обратный порядок (Little-endian)
        bsPeriodTime = (bsPeriodTime << 8) | value[i];
    }
    bsPeriodTime &= 0x000000FFFFFFFFFF;
}

// Установить значение периода синхронизации БС:
void TimeProvider::setBsPeriodWidth(const uint32_t value) {
    bsPeriodWidth = value;
}

// Установить значение разницы во времени между ВУ и БС:
void TimeProvider::setBsTimeDelta(const int64_t value)
{
    bsTimeDelta = value;
}

// Установить значение времени БС по приходу сообщения 11.1:
void TimeProvider::setBsTimePPU(const int64_t value)
{
    bsTimePPU = value;
}

// Получить номер текущего периода БС:
uint32_t TimeProvider::getBsPeriodNumber() const {
    return bsPeriodNumber;
}

// Получить время текущего периода БС:
int64_t TimeProvider::getBsPeriodTime() const {
    return bsPeriodTime;
}

// Получить значение периода синхронизации БС:
int64_t TimeProvider::getBsPeriodWidth() const {
    return bsPeriodWidth;
}

// Получить значение разницы во времени между ВУ и БС:
int64_t TimeProvider::getBsTimeDelta() const {
    return bsTimeDelta;
}

// Получить значение разницы во времени между предыдущим
// сообщением 11.1 и текущим временем БС:
int64_t TimeProvider::getBsDeltaPPU() const {
    return getNs() / 100 - TimeProvider::instance().getBsTimeDelta() - bsTimePPU;
}

void TimeProvider::initTimes() {
    int64_t time_now = getMs();
    TimeProvider::instance().setSysOnTimeReq(time_now);
    TimeProvider::instance().setPbuTimeDelta(0);
    TimeProvider::instance().setPbuTimeKorr(0);
}

void TimeProvider::setSysOnTimeReq(const int64_t time) {
    sysOnTimeReq = time;
}

void TimeProvider::setPbuTimeDelta(const int64_t time) {
    pbuTimeDelta = time;
}

void TimeProvider::setPbuTimeKorr(const int64_t time) {
    pbuTimeKorr = time;
}

int64_t TimeProvider::getSysOnTimeReq() const {
    return sysOnTimeReq;
}

int64_t TimeProvider::getPbuTimeDelta() const {
    return pbuTimeDelta;
}

int64_t TimeProvider::getPbuTimeKorr() const {
    return pbuTimeKorr;
}

// Синхронизировать время с БС:
void TimeProvider::syncTimeBS() {
    int64_t timeDelta = getNs() / 100 -
            TimeProvider::instance().getBsPeriodTime();
    TimeProvider::instance().setBsTimeDelta(timeDelta);
}

// Синхронизировать время с ПБУ:
void TimeProvider::syncTimePBU(uint32_t timeRequest1,
                               uint32_t timeRequest2,
                               uint32_t timeAnswer1,
                               uint32_t timeAnswer2) {

    // Объединяем два 32-битных значения в 64-битное время
    int64_t timePBU = static_cast<int64_t>(
                static_cast<uint64_t>(timeAnswer2) << 32)
                                | timeAnswer1;
    int64_t timePBURequest = static_cast<int64_t>(
                static_cast<uint64_t>(timeRequest2) << 32)
                                | timeRequest1;

    // Вычисляем коррекцию времени
    int64_t sys_time_now = getMs();
    int64_t sys_time_req = TimeProvider::instance().getSysOnTimeReq();
    int64_t _pbuTimeDelta = timePBU - sys_time_now;
    int64_t _pbuTimeKorr = sys_time_now - sys_time_req;
    TimeProvider::instance().setPbuTimeDelta(_pbuTimeDelta);
    TimeProvider::instance().setPbuTimeKorr(_pbuTimeKorr);
    qDebug() << "Delta time between VU and PBU ="
             << timePBU - timePBURequest << "ms";
//    qDebug() << "pbuTimeDelta =" << _pbuTimeDelta;
//    qDebug() << "pbuTimeKorr =" << _pbuTimeKorr;
}

// Запрос времени ПБУ для отправки сообщения на ПБУ
uint64_t TimeProvider::getTimeForRequestPBU() {
    int64_t _sysOnTimeReq = getMs();
    TimeProvider::instance().setSysOnTimeReq(_sysOnTimeReq);
    int64_t _pbuTimeDelta = TimeProvider::instance().getPbuTimeDelta();
    int64_t _pbuTimeKorr = TimeProvider::instance().getPbuTimeKorr();
    uint64_t resultTime = static_cast<uint64_t>(
            _sysOnTimeReq + _pbuTimeDelta + _pbuTimeKorr
       );
//    qDebug() << "sysOnTimeReq =" << _sysOnTimeReq;
//    qDebug() << "pbuTimeDelta =" << _pbuTimeDelta;
//    qDebug() << "pbuTimeKorr =" << _pbuTimeKorr;
//    qDebug() << "resultTime =" << resultTime;
    return resultTime;
}

// Запрос времени БС для отправки сообщения на БС
uint64_t TimeProvider::getTimeForRequestBS() {
    uint64_t _timeBS = static_cast<uint64_t>(getNs() / 100 -
            TimeProvider::instance().getBsTimeDelta());
    return _timeBS;
}
