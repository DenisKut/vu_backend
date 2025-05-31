#pragma once
#include "qmutex.h"
#include "utils/env_loader/EnvLoader.h"
#include <cstdint>
#include <cmath>     // Для std::round
#include <QtGlobal>  // Для qBound
#include <QString>
#include <QHash>

/**
 * @file
 * @brief Заголовочный файл с утилитами преобразования типов и конфигурацией
 * приложения
 */

/**
 * @ingroup Utils
 * @brief Inline функции для безопасного приведения типов
 */
static inline int64_t from_timeBS(const uint8_t value[5]) {
    int64_t _bsTime;
    for (int i = 4; i >= 0; --i) { // Обратный порядок (Little-endian)
        _bsTime = (_bsTime << 8) | value[i];
    }
    _bsTime &= 0x000000FFFFFFFFFF;

    return _bsTime;
}     ///< @brief Получение времени БС из пакета синхронизации от БС

static inline double from_anglePPU(const uint8_t in[3]) {
    // Предвычисление константы
    static constexpr double SCALE = 360.0 / 65536.0;

    // Объединение операций сдвига (компилятор оптимизирует в одну инструкцию)
    const int32_t tmp = (static_cast<int32_t>(in[2]) << 16)
                      | (static_cast<int32_t>(in[1]) << 8)
                      | static_cast<int32_t>(in[0]);

    // Расширение знака через арифметический сдвиг (1 инструкция процессора)
    const int32_t out = (tmp << 8) >> 8;

    // Прямое умножение на предвычисленную константу
    return static_cast<double>(out) * SCALE;
}     ///< @brief Приведение к градусам из 3-байтового массива с дискретами

static inline void to_anglePPU(double value, uint8_t out[3]) {
    // Предвычисление константы
    static constexpr double INV_SCALE = 65536.0 / 360.0;
    double scaled = value * INV_SCALE;

    // Ограничение диапазона 24-битного знакового целого
    constexpr int32_t MIN_VAL = -0x800000;
    constexpr int32_t MAX_VAL =  0x7FFFFF;

    // Округление и ограничение значения
    int32_t ival = qBound(MIN_VAL,
                          static_cast<int32_t>(std::round(scaled)),
                          MAX_VAL);

    // Усечение до 24 бит
    ival &= 0x00FFFFFF;

    // Упаковка в массив
    out[0] = static_cast<uint8_t>(ival);
    out[1] = static_cast<uint8_t>(ival >> 8);
    out[2] = static_cast<uint8_t>(ival >> 16);
}     ///< @brief Приведение к дискретам от градусов и упаковка в массив

template<typename T>
static inline float to_float(T x) {
    return static_cast<float>(x);
}     ///< @brief Приведение к дробному одинарной точности

template<typename T>
static inline double to_double(T x) {
    return static_cast<double>(x);
}     ///< @brief Приведение к дробному двойной точности

template<typename T>
static inline uint64_t to_uint64(T x) {
    return static_cast<uint64_t>(x);
}     ///< @brief Приведение к 64-битному беззнаковому целому

template<typename T>
static inline uint32_t to_uint32(T x) {
    return static_cast<uint32_t>(x);
}     ///< @brief Приведение к 32-битному беззнаковому целому

template<typename T>
static inline uint16_t to_uint16(T x) {
    return static_cast<uint16_t>(x);
}     ///< @brief Приведение к 16-битному беззнаковому целому

template<typename T>
static inline uint8_t  to_uint8(T x)  {
    return static_cast<uint8_t>(x);
}      ///< @brief Приведение к 8-битному беззнаковому целому

/**
 * @brief Функции преобразования в QString
 * @details Поддерживаются различные типы исходных данных:
 * - C-строка
 * - std::string
 * - Сам QString (возвращается без изменений)
 */
static inline QString to_qstring(const char* x) { return QString::fromUtf8(x); }
static inline QString to_qstring(const std::string& x) { return QString::
        fromStdString(x); }
static inline QString to_qstring(const QString& x) { return x; }

/**
 * @brief Типы данных для протокола 0x05
 * @enum PBU_0x05_TypeData
 */
enum class PBU_0x05_TypeData : uint8_t {
    GEOPOSITION = 1,  ///< Данные геопозиции (широта/долгота)
    COORDINATES = 2    ///< Координаты точки стояния в системе комплекса
};

/**
 * @class AppConfig
 * @brief Класс конфигурации приложения (реализован как синглтон)
 *
 * @details Содержит:
 * - Параметры сетевых подключений
 * - Настройки протоколов взаимодействия
 * - Состояния модулей системы
 *
 * @warning Не поддерживает копирование и присваивание
 */
class AppConfig {
public:
    AppConfig(const AppConfig&) = delete;            ///< Запрет копирования
    AppConfig& operator=(const AppConfig&) = delete; ///< Запрет присваивания

    /**
     * @brief Получение экземпляра синглтона
     * @return Единственный экземпляр конфигурации
     */
    static AppConfig& instance() {
        static AppConfig config;
        return config;
    }

    // =============================================
    //  Константы и параметры протоколов
    // =============================================

    /**
     * @enum ReceiptCodes
     * @brief Перечислитель значений команд для задания значений в ответных
     * квитанциях
     */
    enum class ReceiptCodes: uint16_t {
        EXECUTED_WITHOUT_ERROR  = 0,    ///< - "0": команда выполнена без
                                        ///< ошибок;
        IN_PROGRESS             = 1,    ///< - "1": команда в процессе
                                        ///< выполнения;
        INVALID_PARAMETERS      = 2,    ///< - "2": некорректные параметры;
        EXECUTION_INTERRUPTED   = 3,    ///< - "3": выполнение команды прервано
                                        ///< новой командой управления либо
                                        ///< отменено оператором;
        EQUIPMENT_FAULTY        = 4,    ///< - "4": команда не выполнена по
                                        ///< причине неисправности оборудования;
        FAILED_INTERNAL_ERROR   = 5,    ///< - "5": команда не выполнена из-за
                                        ///< внутренней ошибки;
        UNKNOWN_MESSAGE_TYPE    = 6,    ///< - "6": неизвестный тип сообщения;
        REJECTED_RAF            = 7,    ///< - "7": команда отклонена, так как
                                        ///< РАФ выключен;
        EQUIPMENT_NOT_READY     = 8,    ///< - "8": команда не выполнена по
                                        ///< причине неготовности оборудования;
        REJECTED_LOCAL          = 9,    ///< - "9": команда не выполнена, так
                                        ///< как модуль находится в местном
                                        ///< режиме;
        REJECTED_RAF_WGHA       = 10,   ///< - "10": команда не выполнена, так
                                        ///< как модуль находится в РАФ и/или в
                                        ///< РГДВ;
        REJECTED_TP             = 11,   ///< - "11": команда не выполнена, так
                                        ///< как модуль находится в ТП;
        UNKNOWN_ERROR           = 127,  ///< - "127": неизвестная ошибка.
    };

    /// @brief Значение по умолчанию до подтверждения регистрации
    static constexpr uint8_t PBU_PROTO_HEAD_IDX_MODULE_FIRST = 0xFF;

    /// @brief Максимальное количество элементов в модуле
    /// @details 2 РЛМН
    static constexpr int MAX_ELEMENTS_MODULE = 2;
    /// @name Версии протоколов
    /// @{
    /// Мажорная версия
    uint8_t PBU_PROTO_MAJOR_VERSION() const { return _pbuProtoMajorVersion; }
    /// Минорная версия
    uint8_t PBU_PROTO_MINOR_VERSION() const { return _pbuProtoMinorVersion; }
    /// @}

    /// @name Параметры заголовков протоколов
    /// @{
    /// Идентификатор отправителя
    uint32_t PBU_PROTO_HEAD_SENDER() const {
        QMutexLocker lock(&m_mutex);
        return _pbuProtoHeadSender;
    }
    /// @}

    /// @name Параметры протокола регистрации (0x01)
    /// @{
    /// ID производителя
    uint8_t PBU_PROTO_0X01_ID_MANUF() const { return _pbuProto0x01IdManuf; }
    /// Серийный номер
    uint32_t PBU_PROTO_0X01_SERIAL_NUM() const {
        return _pbuProto0x01SerialNum;
    }
    /// Версия железа (мажор)
    uint8_t PBU_PROTO_0X01_VERS_HARD_MAJ() const {
        return _pbuProto0x01VersHardMaj;
    }
    /// Версия железа (минор)
    uint8_t PBU_PROTO_0X01_VERS_HARD_MIN() const {
        return _pbuProto0x01VersHardMin;
    }
    /// Версия ПО (мажор)
    uint8_t PBU_PROTO_0X01_VERS_PROG_MAJ() const {
        return _pbuProto0x01VersProgMaj;
    }
    /// Версия ПО (минор)
    uint8_t PBU_PROTO_0X01_VERS_PROG_MIN() const {
        return _pbuProto0x01VersProgMin;
    }
    /// @}

    /// @name Параметры

    // *******************************************
    //  Переменные протокола взаимодействия с ПБУ
    // *******************************************

    /// @name Заголовок TCP сообщений
    /// @{
    /// Индекс модуля
    uint8_t PBU_PROTO_HEAD_IDX_MODULE() const {
        QMutexLocker lock(&m_mutex);
        return _pbuProtoHeadIdxModule;
    }
    /// Установить индекс
    void SET_PBU_PROTO_HEAD_IDX_MODULE(uint8_t data) {
        QMutexLocker lock(&m_mutex);
        _pbuProtoHeadIdxModule = data;
    }

    /// Номер пакета
    uint16_t PBU_PROTO_HEAD_IDX_PACK() const {
        QMutexLocker lock(&m_mutex);
        return _pbuProtoHeadIdxPack;
    }
    /// Инкремент номера
    void INCREMENT_PBU_PROTO_HEAD_IDX_PACK() {
        QMutexLocker lock(&m_mutex);
        _pbuProtoHeadIdxPack++;
    }
    /// Флаг ПО АСКУ
    uint8_t PBU_PROTO_HEAD_IS_ASKU() const {
        QMutexLocker lock(&m_mutex);
        return _pbuProtoHeadIsAsku;
    }
    /// Установить флаг
    void SET_PROTO_HEAD_IS_ASKU(bool flag) {
        QMutexLocker lock(&m_mutex);
        _pbuProtoHeadIsAsku = flag ? 1 : 0;
    }
    /// @}

    /// @name Протокол регистрации (0x01)
    /// @{
    /// Флаг обработки информации
    uint8_t PBU_PROTO_0X01_IS_INFO() const {
        return _pbuProto0x01IsInfo; }
    /// Установить флаг
    void SET_PBU_PROTO_0X01_IS_INFO(bool flag) {
        _pbuProto0x01IsInfo = flag ? 1 : 0;
    }

    /// Флаг управления
    uint8_t PBU_PROTO_0X01_IS_CONTROL() const { return _pbuProto0x01IsControl; }
    /// Установить флаг
    void SET_PBU_PROTO_0X01_IS_CONTROL(bool flag) {
        _pbuProto0x01IsControl = flag ? 1 : 0;
    }
    /// @}

    /// @name Протокол геоданных (0x05)
    /// @{
    /// Тип данных позиционирования
    uint8_t PBU_PROTO_0X05_TYPE_DATA() const {
        QMutexLocker lock(&m_mutex);
        return _pbuProto0x05TypeData;
    }

    /**
     * @brief Установка типа данных позиционирования
     * @param type Тип данных из перечисления PBU_0x05_TypeData
     * @throws std::invalid_argument При недопустимом значении
     */
    void SET_PBU_PROTO_0X05_TYPE_DATA(PBU_0x05_TypeData type) {
        QMutexLocker lock(&m_mutex);
        uint8_t value = to_uint8(type);
        if (value != 1 && value != 2) {
            throw std::invalid_argument("Invalid PBU_0x05_TypeData value");
        }
        _pbuProto0x05TypeData = value;
    }

    /// Достоверность данных
    uint8_t PBU_PROTO_0X05_IS_VALID() const {
        QMutexLocker lock(&m_mutex);
        return _pbuProto0x05IsValid;
    }
    /// Установить достоверность
    void SET_PBU_PROTO_0X05_IS_VALID(bool flag) {
        QMutexLocker lock(&m_mutex);
        _pbuProto0x05IsValid = flag ? 1 : 0;
    }

    float PBU_PROTO_0X05_X_LAT_GEO() const {
        QMutexLocker lock(&m_mutex);
        return _pbuProto0x05_X_lat_geo;
    }
    float PBU_PROTO_0X05_X_LAT_COORD() const {
        QMutexLocker lock(&m_mutex);
        return _pbuProto0x05_X_lat_coord;
    }

    float PBU_PROTO_0X05_Y_LONG_GEO() const {
        QMutexLocker lock(&m_mutex);
        return _pbuProto0x05_Y_long_geo;
    }
    float PBU_PROTO_0X05_Y_LONG_COORD() const {
        QMutexLocker lock(&m_mutex);
        return _pbuProto0x05_Y_long_coord;
    }

    float PBU_PROTO_0X05_Z_HEIGHT_GEO() const {
        QMutexLocker lock(&m_mutex);
        return _pbuProto0x05_Z_height_geo;
    }
    float PBU_PROTO_0X05_Z_HEIGHT_COORD() const {
        QMutexLocker lock(&m_mutex);
        return _pbuProto0x05_Z_height_coord;
    }
    /// @}

    /// @name Протокол Статуса модуля (0x24)
    /// @{
    /// typePack последней полученной команды от ПБУ
    uint16_t PBU_PROTO_0X23_ID_COM() {
        QMutexLocker lock(&m_mutex);
        return _pbuProto0x23_IDCom;
    }
    void SET_PBU_PROTO_0X23_ID_COM(uint16_t message_num) {
        QMutexLocker lock(&m_mutex);
        _pbuProto0x23_IDCom = message_num;
    }

    /// Код выполнения команды ReceiptCodes
    uint16_t PBU_PROTO_0X23_CODE() {
        QMutexLocker lock(&m_mutex);
        return _pbuProto0x23_code;
    }
    void SET_PBU_PROTO_0X23_CODE(uint16_t code) {
        QMutexLocker lock(&m_mutex);
        _pbuProto0x23_code = code;
    }
    /// @}


    /// @name Протокол Статуса модуля (0x24)
    /// @{
    /// Состояние модуля
    uint8_t PBU_PROTO_0X24_STATUS() const {
        QMutexLocker lock(&m_mutex);
        return _pbuProto0x24_status;
    }

    void SET_PBU_PROTO_0X24_STATUS(uint8_t data) {
        QMutexLocker lock(&m_mutex);
        _pbuProto0x24_status = data;
    }

    /// Статус работы
    uint8_t PBU_PROTO_0X24_WORK() const {
        QMutexLocker lock(&m_mutex);
        return _pbuProto0x24_work;
    }
    void SET_PBU_PROTO_0X24_WORK(uint8_t data) {
        QMutexLocker lock(&m_mutex);
        _pbuProto0x24_work = data;
    }

    /// Признак включения РГДВ
    uint8_t PBU_PROTO_0X24_IS_RGDV() const {
        QMutexLocker lock(&m_mutex);
        return _pbuProto0x24_isRGDV;
    }
    void SET_PBU_PROTO_0X24_IS_RGDV(bool flag) {
        QMutexLocker lock(&m_mutex);
        _pbuProto0x24_isRGDV = flag ? 1 : 0;
    }

    /// Признак включения РАФ
    uint8_t PBU_PROTO_0X24_IS_RAF() const {
        QMutexLocker lock(&m_mutex);
        return _pbuProto0x24_isRAF;
    }
    void SET_PBU_PROTO_0X24_IS_RAF(bool flag) {
        QMutexLocker lock(&m_mutex);
        _pbuProto0x24_isRAF = flag ? 1 : 0;
    }

    /// Признак включения местного режима
    uint8_t PBU_PROTO_0X24_IS_LOCAL() const {
        QMutexLocker lock(&m_mutex);
        return _pbuProto0x24_isLocal;
    }
    void SET_PBU_PROTO_0X24_IS_LOCAL(bool flag) {
        QMutexLocker lock(&m_mutex);
        _pbuProto0x24_isLocal = flag ? 1 : 0;
    }

    /// Признак имитатора
    uint8_t PBU_PROTO_0X24_IS_IMIT() const {
        QMutexLocker lock(&m_mutex);
        return _pbuProto0x24_isImit;
    }
    void SET_PBU_PROTO_0X24_IS_IMIT(bool flag) {
        QMutexLocker lock(&m_mutex);
        _pbuProto0x24_isImit = flag ? 1 : 0;
    }

    /// Признак наличия транспортного положения
    uint8_t PBU_PROTO_0X24_HAS_TP() const {
        QMutexLocker lock(&m_mutex);
        return _pbuProto0x24_hasTP;
    }
    void SET_PBU_PROTO_0X24_HAS_TP(bool flag) {
        QMutexLocker lock(&m_mutex);
        _pbuProto0x24_hasTP = flag ? 1 : 0;
    }

    /// Признак нахождения в транспортном положении
    uint8_t PBU_PROTO_0X24_IS_TP() const {
        QMutexLocker lock(&m_mutex);
        return _pbuProto0x24_isTP;
    }
    void SET_PBU_PROTO_0X24_IS_TP(bool flag) {
        QMutexLocker lock(&m_mutex);
        _pbuProto0x24_isTP = flag ? 1 : 0;
    }

    /// Признак нахождения в РП
    uint8_t PBU_PROTO_0X24_IS_WP() const {
        QMutexLocker lock(&m_mutex);
        return _pbuProto0x24_isWP;
    }
    void SET_PBU_PROTO_0X24_IS_WP(bool flag) {
        QMutexLocker lock(&m_mutex);
        _pbuProto0x24_isWP = flag ? 1 : 0;
    }

    /// Признак достоверности флага ТП
    uint8_t PBU_PROTO_0X24_IS_TP_VALID() const {
        QMutexLocker lock(&m_mutex);
        return _pbuProto0x24_isTPValid;
    }
    void SET_PBU_PROTO_0X24_IS_TP_VALID(bool flag) {
        QMutexLocker lock(&m_mutex);
        _pbuProto0x24_isTPValid = flag ? 1 : 0;
    }

    /// Признак достоверности флага РП
    uint8_t PBU_PROTO_0X24_IS_WP_VALID() const {
        QMutexLocker lock(&m_mutex);
        return _pbuProto0x24_isWPValid;
    }
    void SET_PBU_PROTO_0X24_IS_WP_VALID(bool flag) {
        QMutexLocker lock(&m_mutex);
        _pbuProto0x24_isWPValid = flag ? 1 : 0;
    }

    /// Статус положения
    uint8_t PBU_PROTO_0X24_STATUS_TWP() const {
        QMutexLocker lock(&m_mutex);
        return _pbuProto0x24_statusTWP;
    }
    void SET_PBU_PROTO_0X24_STATUS_TWP(bool flag) {
        QMutexLocker lock(&m_mutex);
        _pbuProto0x24_statusTWP = flag ? 1 : 0;
    }

    /// Режим функционирования
    uint16_t PBU_PROTO_0X24_MODE() const {
        QMutexLocker lock(&m_mutex);
        return _pbuProto0x24_mode;
    }
    void SET_PBU_PROTO_0X24_MODE(uint16_t data) {
        QMutexLocker lock(&m_mutex);
        _pbuProto0x24_mode = data;
    }
    /// @}

    /// @name Параметры Ethernet-пакетов
    /// @{
    /// MAC-адрес получателя Ethernet-пакетов
    uint64_t ETH_PROTO_MAC_DST() const { return _ethProtoMacDst; }
    /// MAC-адрес отправителя Ethernet-пакетов
    uint64_t ETH_PROTO_MAC_SRC() const { return _ethProtoMacSrc; }
    /// Тип протокола Ethernet-пакетов
    uint16_t ETH_PROTO_ETHER_TYPE() const { return _ethProtoEtherType; }
    /// @}

    /**
     * @brief Инициализация параметров из переменных окружения
     * @param env Хэш-таблица с параметрами из .env файла
     */
    void initFromEnv(const EnvLoader& env) {

        // Версии протокола
        this->_pbuProtoMajorVersion = env
                                        .getUInt8("PBU_PROTO_MAJOR_VERSION", 2);
        this->_pbuProtoMinorVersion = env
                                        .getUInt8("PBU_PROTO_MINOR_VERSION", 0);

        // 24-битное значение
        this->_pbuProtoHeadSender =
            env.getUInt32("PBU_PROTO_HEAD_ID_SENDER", 0x524C4E);

        // Параметры сообщения 0x01
        this->_pbuProto0x01IdManuf =
            env.getUInt8("PBU_PROTO_0X01_ID_MANUF", 0x0);
        this->_pbuProto0x01SerialNum =
            env.getUInt32("PBU_PROTO_0X01_SERIAL_NUM", 1);
        this->_pbuProto0x01VersHardMaj =
            env.getUInt8("PBU_PROTO_0X01_VERS_HARD_MAJ", 1);
        this->_pbuProto0x01VersHardMin =
            env.getUInt8("PBU_PROTO_0X01_VERS_HARD_MIN", 0);
        this->_pbuProto0x01VersProgMaj =
            env.getUInt8("PBU_PROTO_0X01_VERS_PROG_MAJ", 1);
        this->_pbuProto0x01VersProgMin =
            env.getUInt8("PBU_PROTO_0X01_VERS_PROG_MIN", 0);

        // Геопозиции (float)
        this->_pbuProto0x05_X_lat_geo =
            env.getFloat("PBU_PROTO_0X05_X_LAT_GEO", 0.0f);
        this->_pbuProto0x05_X_lat_coord =
            env.getFloat("PBU_PROTO_0X05_X_LAT_COORD", 0.0f);
        this->_pbuProto0x05_Y_long_geo =
            env.getFloat("PBU_PROTO_0X05_Y_LONG_GEO", 0.0f);
        this->_pbuProto0x05_Y_long_coord =
            env.getFloat("PBU_PROTO_0X05_Y_LONG_COORD", 0.0f);
        this->_pbuProto0x05_Z_height_geo =
            env.getFloat("PBU_PROTO_0X05_Z_HEIGHT_GEO", 0.0f);
        this->_pbuProto0x05_Z_height_coord =
            env.getFloat("PBU_PROTO_0X05_Z_HEIGHT_COORD", 0.0f);

        // MAC-адреса
        this->_ethProtoMacDst =
            env.getMacAddressAsUInt64("ETH_PROTO_MAC_DST", 0xFFFFFFFFFFFF);
        this->_ethProtoMacSrc =
            env.getMacAddressAsUInt64("ETH_PROTO_MAC_SRC", 0x010203040506);
        this->_ethProtoEtherType =
            env.getUInt16("ETH_PROTO_ETHER_TYPE", 0x2020);
    }

    /**
     * @brief Базовая инициализация параметров протоколов
     */
    void initBase() {
        // Инициализация значений для заголовков сообщений ПБУ
        this->_pbuProtoHeadIdxModule = PBU_PROTO_HEAD_IDX_MODULE_FIRST;
        this->_pbuProtoHeadIdxPack = 0;
        this->_pbuProtoHeadIsAsku = 1;

        // Инициализация значений по телу сообщений ПБУ
        // ---- 0x01 -----
        this->_pbuProto0x01IsInfo = 0;
        this->_pbuProto0x01IsControl = 0;
        // ---- 0x05 -----
        this->_pbuProto0x05TypeData = 1;
        this->_pbuProto0x05IsValid = 1;

        // ---- 0x24 -----
        this->_pbuProto0x24_status = 1;     ///< "1": Норма
        this->_pbuProto0x24_work = 1;       ///< "1": включен
        this->_pbuProto0x24_isRGDV = 1;     ///< модуль находится в РГДВ
        this->_pbuProto0x24_isRAF = 1;      ///< модуль находится в РАФ
        this->_pbuProto0x24_isLocal = 0;    ///< модуль находится в удалённом
                                            ///< режиме управления

        this->_pbuProto0x24_isImit = 0;     ///< Боевой режим
        this->_pbuProto0x24_hasTP = 0;      ///< Модуль не находится в
                                            ///< транспортном положении
        this->_pbuProto0x24_isTP = 0;       ///< Модуль не находится в ТП
        this->_pbuProto0x24_isWP = 0;       ///< Модуль не находится в ТП и РП
        this->_pbuProto0x24_isTPValid = 0;  ///< Не удалось определить
        this->_pbuProto0x24_isWPValid = 0;  ///< Не удалось определить
        this->_pbuProto0x24_statusTWP = 1;   ///< "1": норма

        this->_pbuProto0x24_mode = 0; // "0": ожидание команд управления
    }

    /**
     * @brief Полная инициализация конфигурации
     * @param env Хэш-таблица с параметрами из .env файла
     */
    void init(const EnvLoader& env) {
        initFromEnv(env);
        initBase();
    }

    /// Увеличение счётчика сообщений на РМО
    uint32_t increaseRmoCounter() {
        _rmoCounter++;
        return _rmoCounter;
    }

    uint32_t getRmoCounter() {
        return _rmoCounter;
    }

    // *******************************************
    //  Текущее состояние ППУ
    // *******************************************

    /**
     * @enum ModulePPU_num
     * @brief Перечень номеров ППУ
     */
    enum ModulePPU_num {
        ModulePPU_num_RLMN_1            = 0, ///< РЛМН 1
        ModulePPU_num_RLMN_2            = 1, ///< РЛМН 2
        ModulePPU_num_SHPU              = 2, ///< ШПУ

        ModulePPU_num_Size              = 3  ///< Размер списка ППУ
    };

    /**
     * @enum ModulePPU_lastCommand
     * @brief Перечень команд, полученных ППУ
     */
    enum ModulePPU_lastCommand {
        ModulePPU_lastCommand_Stop      = 0, ///< Команда остановки ППУ
        ModulePPU_lastCommand_SwitchOn  = 2, ///< Команда включения ППУ
        ModulePPU_lastCommand_Target    = 4, ///< Целеуказание
        ModulePPU_lastCommand_Idle      = 6  ///< Команды не поступало
    };

    /**
     * @enum ModulePPU_state
     * @brief Перечень состояний ППУ
     */
    enum ModulePPU_state {
        ModulePPU_state_TurnedOff       = 0, ///< ППУ выключено
        ModulePPU_state_TurnedOn        = 1, ///< ППУ включено
        ModulePPU_state_TurnOff         = 2, ///< Выполняется отключение ППУ
        ModulePPU_state_Targeting       = 4, ///< Выполняется отработка ЦУ
        ModulePPU_state_NoCommand       = 8  ///< Отсутствует внешний обмен
    };

    /**
     * @enum ModulePPU_isRemote
     * @brief Перечень признаков внешнего управления
     */
    enum ModulePPU_isRemote {
        ModulePPU_isRemote_defaulted    = 0,    ///< Не выполняется никаких
                                                ///< команд от ПО СОИ

        ModulePPU_isRemote_search       = 1,    ///< Выполняется поиск цели по
                                                ///< команде «Целеуказание»

        ModulePPU_isRemote_support      = 2     ///< Выполняется сопровождение
                                                ///< цели по команде
                                                ///< «Целеуказание»
    };

    /**
     * @enum ModulePPU_controlMode
     * @brief Список режимов управления ППУ
     */
    enum ModulePPU_controlMode {
        ModulePPU_controlMode_point = 0,
        ModulePPU_controlMode_speed = 1
    };

    /**
     * @struct ModulePPU
     * @brief Структура, содержащая состояние ППУ
     */
    struct ModulePPU {
        uint8_t     lastCommand;    ///< Код последней полученной команды
        uint8_t     state;          ///< Состояние
        uint8_t     ispr;           ///< Исправность оборудования
        double      az;             ///< Текущая координата по азимуту,
                                    ///<   отсчётов (65536/360 = 1 град)
        uint64_t    azDelay;        ///< Задержка измерения координаты
                                    ///<   по азимуту, мс (20 = 1 мс)
        double      an;             ///< Текущая координата по углу места,
                                    ///<   отсчётов (65536/360 = 1 град)
        uint64_t    anDelay;        ///< Задержка измерения координаты
                                    ///<   по углу места, мс (20 = 1 мс)
        uint8_t     borders;        ///< Состояние границ
        uint64_t    waitCommands;   ///< Время ожидания команд, мс (1 = 5 мс)
        uint64_t    waitTarget;     ///< Время ожидания ЦУ, мс (1 = 5 мс)

        double      azTarget;
        double      anTarget;

        double      azTargetVel;
        double      anTargetVel;

        uint8_t     controlMode;

        /// ===========================
        /// Поля для сообщений о состояниях с ПБУ
        /// ===========================

        uint8_t isRemote;           ///< Признак внешнего управления

        ModulePPU()
            : lastCommand(0),
            state(0),
            ispr(0),
            az(0),
            azDelay(0),
            an(0),
            anDelay(0),
            borders(0),
            waitCommands(0),
            waitTarget(0),
            azTarget(0),
            anTarget(0),
            azTargetVel(0),
            anTargetVel(0),
            controlMode(ModulePPU_controlMode_point),
            isRemote(0)
        {};
    };

    /// @name Запись структуры состояний ППУ
    /// @{
    /// Код последней полученной команды
    void setModulePPU_lastCommand(ModulePPU_num moduleNum, uint8_t value) {
        QMutexLocker lock(&m_mutex);
        ModulePPU_mas[moduleNum]->lastCommand = value;
    }
    /// Состояние
    void setModulePPU_state(ModulePPU_num moduleNum, uint8_t value) {
        QMutexLocker lock(&m_mutex);
        ModulePPU_mas[moduleNum]->state = value;
    }
    /// Исправность оборудования
    void setModulePPU_ispr(ModulePPU_num moduleNum, uint8_t value) {
        QMutexLocker lock(&m_mutex);
        ModulePPU_mas[moduleNum]->ispr = value;
    }
    /// Текущая координата по азимуту (65536/360 = 1 град)
    void setModulePPU_az(ModulePPU_num moduleNum, uint8_t *value) {
        QMutexLocker lock(&m_mutex);
        ModulePPU_mas[moduleNum]->az = from_anglePPU(value);
    }
    /// Задержка измерения координаты по азимуту (20 = 1 мс = 10000 тактов)
    void setModulePPU_azDelay(ModulePPU_num moduleNum, uint8_t value) {
        QMutexLocker lock(&m_mutex);
        ModulePPU_mas[moduleNum]->azDelay = to_uint64(value) * 500; // Преобразование в такты БС
    }
    /// Текущая координата по углу места (65536/360 = 1 град)
    void setModulePPU_an(ModulePPU_num moduleNum, uint8_t *value) {
        QMutexLocker lock(&m_mutex);
        ModulePPU_mas[moduleNum]->an = from_anglePPU(value);
    }
    /// Задержка измерения координаты по углу места (20 = 1 мс = 10000 тактов)
    void setModulePPU_anDelay(ModulePPU_num moduleNum, uint8_t value) {
        QMutexLocker lock(&m_mutex);
        ModulePPU_mas[moduleNum]->anDelay = to_uint64(value) * 500; // Преобразование в такты БС
    }
    /// Состояние границ
    void setModulePPU_borders(ModulePPU_num moduleNum, uint8_t value) {
        QMutexLocker lock(&m_mutex);
        ModulePPU_mas[moduleNum]->borders = value;
    }
    /// Время ожидания команд (1 = 5 мс = 50000 тактов)
    void setModulePPU_waitCommands(ModulePPU_num moduleNum, uint8_t value) {
        QMutexLocker lock(&m_mutex);
        ModulePPU_mas[moduleNum]->waitCommands = to_uint64(value) * 50000; // Преобразование в такты БС
    }
    /// Время ожидания ЦУ (1 = 5 мс = 50000 тактов)
    void setModulePPU_waitTarget(ModulePPU_num moduleNum, uint8_t value) {
        QMutexLocker lock(&m_mutex);
        ModulePPU_mas[moduleNum]->waitTarget = to_uint64(value) * 50000; // Преобразование в такты БС
    }

    /// Признак внешнего управления
    void setModulePPU_isRemote(ModulePPU_num moduleNum,
                               ModulePPU_isRemote value) {
        QMutexLocker lock(&m_mutex);
        ModulePPU_mas[moduleNum]->isRemote = to_uint8(value);
    }

    void setModulePPU_controlMode(ModulePPU_num moduleNum,
                                  ModulePPU_controlMode value) {
        QMutexLocker lock(&m_mutex);
        ModulePPU_mas[moduleNum]->controlMode = value;
    }

    /// ЦУ для ППУ
    void setModulePPU_CU(ModulePPU_num moduleNum,
                         double azTarget,    double anTarget,
                         double azTargetVel, double anTargetVel) {
        QMutexLocker lock(&m_mutex);
        ModulePPU_mas[moduleNum]->azTarget = azTarget;
        ModulePPU_mas[moduleNum]->anTarget = anTarget;
        ModulePPU_mas[moduleNum]->azTargetVel = azTargetVel;
        ModulePPU_mas[moduleNum]->anTargetVel = anTargetVel;
    }
    /// @}

    /// @name Чтение структуры состояний ППУ
    /// @{
    /// Код последней полученной команды
    uint8_t getModulePPU_lastCommand(ModulePPU_num moduleNum) {
        QMutexLocker lock(&m_mutex);
        return ModulePPU_mas[moduleNum]->lastCommand;
    }
    /// Состояние
    uint8_t getModulePPU_state(ModulePPU_num moduleNum) {
        QMutexLocker lock(&m_mutex);
        return ModulePPU_mas[moduleNum]->state;
    }
    /// Исправность оборудования
    uint8_t getModulePPU_ispr(ModulePPU_num moduleNum) {
        QMutexLocker lock(&m_mutex);
        return ModulePPU_mas[moduleNum]->ispr;
    }
    /// Текущая координата по азимуту (65536/360 = 1 град)
    double getModulePPU_az(ModulePPU_num moduleNum) {
        QMutexLocker lock(&m_mutex);
        return ModulePPU_mas[moduleNum]->az;
    }
    /// Задержка измерения координаты по азимуту (20 = 1 мс = 10000 тактов)
    uint64_t getModulePPU_azDelay(ModulePPU_num moduleNum) {
        QMutexLocker lock(&m_mutex);
        return ModulePPU_mas[moduleNum]->azDelay;
    }
    /// Текущая координата по углу места (65536/360 = 1 град)
    double getModulePPU_an(ModulePPU_num moduleNum) {
        QMutexLocker lock(&m_mutex);
        return ModulePPU_mas[moduleNum]->an;
    }
    /// Задержка измерения координаты по углу места (20 = 1 мс = 10000 тактов)
    uint64_t getModulePPU_anDelay(ModulePPU_num moduleNum) {
        QMutexLocker lock(&m_mutex);
        return ModulePPU_mas[moduleNum]->anDelay;
    }
    /// Состояние границ
    uint8_t getModulePPU_borders(ModulePPU_num moduleNum) {
        QMutexLocker lock(&m_mutex);
        return ModulePPU_mas[moduleNum]->borders;
    }
    /// Время ожидания команд (1 = 5 мс = 50000 тактов)
    uint64_t getModulePPU_waitCommands(ModulePPU_num moduleNum) {
        QMutexLocker lock(&m_mutex);
        return ModulePPU_mas[moduleNum]->waitCommands;
    }
    /// Время ожидания ЦУ (1 = 5 мс = 50000 тактов)
    uint64_t getModulePPU_waitTarget(ModulePPU_num moduleNum) {
        QMutexLocker lock(&m_mutex);
        return ModulePPU_mas[moduleNum]->waitTarget;
    }

    /// Признак внешнего управления
    uint8_t getModulePPU_isRemote(ModulePPU_num moduleNum) {
        QMutexLocker lock(&m_mutex);
        return ModulePPU_mas[moduleNum]->isRemote;
    }

    /// Координата ЦУ по азимуту
    double getModulePPU_azTarget(ModulePPU_num moduleNum) {
        QMutexLocker lock(&m_mutex);
        return ModulePPU_mas[moduleNum]->azTarget;
    }

    /// Координата ЦУ по углу места
    double getModulePPU_anTarget(ModulePPU_num moduleNum) {
        QMutexLocker lock(&m_mutex);
        return ModulePPU_mas[moduleNum]->anTarget;
    }

    /// Скорость ЦУ по азимуту
    double getModulePPU_azTargetVel(ModulePPU_num moduleNum) {
        QMutexLocker lock(&m_mutex);
        return ModulePPU_mas[moduleNum]->azTargetVel;
    }

    /// Скорость ЦУ по углу места
    double getModulePPU_anTargetVel(ModulePPU_num moduleNum) {
        QMutexLocker lock(&m_mutex);
        return ModulePPU_mas[moduleNum]->anTargetVel;
    }

    uint8_t getModulePPU_controlMode(ModulePPU_num moduleNum) {
        QMutexLocker lock(&m_mutex);
        return ModulePPU_mas[moduleNum]->controlMode;
    }

    /// @}


private:
    AppConfig() = default; ///< Приватный конструктор

    mutable QMutex m_mutex{QMutex::Recursive};
    /// @name Параметры сети
    /// @{
    std::string _tcpHostIP;         ///< IP адрес TCP сервера
                                    ///
    std::string _ethInterfaceName;  ///< Имя сетевого интерфейса
    /// @}

    /// @name Версии протоколов
    /// @{
    uint8_t _pbuProtoMajorVersion;  ///< Мажорная версия протокола
                                    ///
    uint8_t _pbuProtoMinorVersion;  ///< Минорная версия протокола
    /// @}

    /// @name Параметры заголовков
    /// @{
    uint32_t _pbuProtoHeadSender: 24;   ///< Идентификатор отправителя (24 бита)
                                        ///
    uint8_t _pbuProtoHeadIdxModule;     ///< Индекс модуля в комплексе
                                        ///
    uint16_t _pbuProtoHeadIdxPack;      ///< Сквозной номер пакета
                                        ///
    uint8_t _pbuProtoHeadIsAsku: 1;     ///< Флаг использования ПО АСКУ (1 бит)
    /// @}

    /// @name Параметры протокола 0x01
    /// @{
    uint8_t _pbuProto0x01IdManuf;           ///< ID производителя
                                            ///
    uint32_t _pbuProto0x01SerialNum: 24;    ///< Серийный номер (24 бита)
                                            ///
    uint8_t _pbuProto0x01VersHardMaj;       ///< Мажорная версия железа
                                            ///
    uint8_t _pbuProto0x01VersHardMin;       ///< Минорная версия железа
                                            ///
    uint8_t _pbuProto0x01VersProgMaj:7;     ///< Мажорная версия ПО (7 бит)
                                            ///
    uint8_t _pbuProto0x01IsInfo:1;          ///< Флаг обработки информации
                                            ///< (1 бит)
                                            ///
    uint8_t _pbuProto0x01VersProgMin:7;     ///< Минорная версия ПО (7 бит)
                                            ///
    uint8_t _pbuProto0x01IsControl:1;       ///< Флаг управления (1 бит)
    /// @}

    /// @name Параметры протокола 0x05
    /// @{
    uint8_t _pbuProto0x05TypeData:3;    ///< Тип данных позиционирования
                                        /// (3 бита)
    uint8_t _pbuProto0x05IsValid:1;     ///< Достоверность данных (1 бит)
    /// @}

    /**
     * @struct Позиционирование
     * @brief Структура данных позиционирования модуля
     *
     * @details Может содержать либо географические координаты, либо координаты
     * точки стояния:
     *
     * | Поле       | Геопозиция                     | Точка стояния           |
     * |------------|--------------------------------|-------------------------|
     * | X_lat      | Широта [±90°]                  | X-координата (м)        |
     * | Y_long     | Долгота [±180°]                | Y-координата (м)        |
     * | Z_height   | Высота над уровнем моря (м)    | Z-координата (м)        |
     */
    float _pbuProto0x05_X_lat_geo;      ///< Координата X/Широта (Геопозиция)
                                        ///
    float _pbuProto0x05_X_lat_coord;    ///< Координата X/Широта (Координаты в
                                        ///< системе комплекса)

    float _pbuProto0x05_Y_long_geo;     ///< Координата Y/Долгота (Геопозиция)
                                        ///
    float _pbuProto0x05_Y_long_coord;   ///< Координата Y/Долгота (Координаты в
                                        /// системе комплекса)

    float _pbuProto0x05_Z_height_geo;   ///< Координата Z/Высота (Геопозиция)
                                        ///
    float _pbuProto0x05_Z_height_coord; ///< Координата Z/Высота (Координаты в
                                        /// системе комплекса)

    /// @name Параметры протокола 0x23
    /// @{
    uint16_t _pbuProto0x23_IDCom;       ///< typePack последней полученной
                                        ///< команды Управления от ПБУ

    uint16_t _pbuProto0x23_code;        ///< Код выполнения команды
    /// @}

    /// @name Параметры протокола 0x24
    /// @{
    uint8_t _pbuProto0x24_status: 3;    ///< Состояние модуля
    uint8_t _pbuProto0x24_work: 3;      ///< Статус работы
    uint8_t _pbuProto0x24_isRGDV: 1;    ///< Признак включения РГДВ
    uint8_t _pbuProto0x24_isRAF: 1;     ///< Признак включения РАФ
    uint8_t _pbuProto0x24_isLocal: 1;   ///< Признак включения местного режима
    uint8_t _pbuProto0x24_isImit: 1;    ///< Признак имитатора
    uint8_t _pbuProto0x24_hasTP: 1;     ///< Признак наличия транспортного
                                        ///< положения
                                        ///
    uint8_t _pbuProto0x24_isTP: 1;      ///< Признак нахождения в транспортном
                                        ///< положении
    uint8_t _pbuProto0x24_isWP: 1;      ///< Признак нахождения в РП
    uint8_t _pbuProto0x24_isTPValid: 1; ///< Признак достоверности флага ТП
    uint8_t _pbuProto0x24_isWPValid: 1; ///< Признак достоверности флага РП
    uint8_t _pbuProto0x24_statusTWP: 1; ///< Статус положения

    uint16_t _pbuProto0x24_mode;        ///< Режим функционирования
    /// @}

    /// @name Параметры Ethernet-пакетов
    /// @{
    uint64_t _ethProtoMacDst: 48;       ///< MAC получателя Ethernet-пакетов
    uint64_t _ethProtoMacSrc: 48;       ///< MAC отправителя Ethernet-пакетов
    uint16_t _ethProtoEtherType;        ///< Тип протокола Ethernet-пакетов
    /// @}

    /// Счётчик сообщений на РМО (для отладки):
    uint32_t _rmoCounter = 0;

    /// @name Объекты для хранения текущих состояний ППУ
    /// @{
    ModulePPU ppuRLMN1,             ///< ППУ РЛМН 1
        ppuRLMN2,                   ///< ППУ РЛМН 2
        ppuSHPU;                    ///< ППУ ШПУ
    /// @}

    ModulePPU* ModulePPU_mas[ModulePPU_num_Size] = {
        &ppuRLMN1, &ppuRLMN2, &ppuSHPU
    };                                  ///< Массив указателей на ППУ
};
    // uint8_t _pbuProto0x05;
