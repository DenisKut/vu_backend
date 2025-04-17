#pragma once
#include "utils/env_loader/EnvLoader.h"
#include <cstdint>
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

    /// @brief Значение по умолчанию до подтверждения регистрации
    static constexpr uint8_t PBU_PROTO_HEAD_IDX_MODULE_FIRST = 0xFF;

    /// @brief Максимальное количество элементов в модуле
    /// @details 2 РЛМН
    static constexpr int MAX_ELEMENTS_MODULE = 2;

    /// @name Методы доступа к параметрам из .env
    /// @warning НЕИЗМЕННЫЕ в рантайме
    /// @{
    /// IP TCP сервера
    std::string TCP_HOST_IP() const { return _tcpHostIP; }
    /// Порт TCP сервера
    int TCP_HOST_PORT() const { return _tcpHostPort; }
    /// Порт UDP
    int UDP_PORT() const { return _udpPort; }
    /// Сетевой интерфейс
    std::string ETH_INTERFACE_NAME() const { return _ethInterfaceName; }
    /// Размер буфера сокета
    int ETH_SOCKET_BUF_SIZE() const { return _ethSocketBuffSize; }
    /// @}

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
    uint32_t PBU_PROTO_HEAD_SENDER() const { return _pbuProtoHeadSender; }
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
    uint8_t PBU_PROTO_HEAD_IDX_MODULE() const { return _pbuProtoHeadIdxModule; }
    /// Установить индекс
    void SET_PBU_PROTO_HEAD_IDX_MODULE(uint8_t data) {
        _pbuProtoHeadIdxModule = data;
    }

    /// Номер пакета
    uint16_t PBU_PROTO_HEAD_IDX_PACK() const { return _pbuProtoHeadIdxPack; }
    /// Инкремент номера
    void INCREMENT_PBU_PROTO_HEAD_IDX_PACK() { _pbuProtoHeadIdxPack++; }
    /// Флаг ПО АСКУ
    uint8_t PBU_PROTO_HEAD_IS_ASKU() const { return _pbuProtoHeadIsAsku; }
    /// Установить флаг
    void SET_PROTO_HEAD_IS_ASKU(bool flag) {
        _pbuProtoHeadIsAsku = flag ? 1 : 0;
    }
    /// @}

    /// @name Протокол регистрации (0x01)
    /// @{
    /// Флаг обработки информации
    uint8_t PBU_PROTO_0X01_IS_INFO() const { return _pbuProto0x01IsInfo; }
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
    uint8_t PBU_PROTO_0X05_TYPE_DATA() const { return _pbuProto0x05TypeData; }

    /**
     * @brief Установка типа данных позиционирования
     * @param type Тип данных из перечисления PBU_0x05_TypeData
     * @throws std::invalid_argument При недопустимом значении
     */
    void SET_PBU_PROTO_0X05_TYPE_DATA(PBU_0x05_TypeData type) {
        uint8_t value = to_uint8(type);
        if (value != 1 && value != 2) {
            throw std::invalid_argument("Invalid PBU_0x05_TypeData value");
        }
        _pbuProto0x05TypeData = value;
    }

    /// Достоверность данных
    uint8_t PBU_PROTO_0X05_IS_VALID() const { return _pbuProto0x05IsValid; }
    /// Установить достоверность
    void SET_PBU_PROTO_0X05_IS_VALID(bool flag) {
        _pbuProto0x05IsValid = flag ? 1 : 0;
    }

    float PBU_PROTO_0X05_X_LAT_GEO() const {
        return _pbuProto0x05_X_lat_geo;
    }
    float PBU_PROTO_0X05_X_LAT_COORD() const {
        return _pbuProto0x05_X_lat_coord;
    }

    float PBU_PROTO_0X05_Y_LONG_GEO() const {
        return _pbuProto0x05_Y_long_geo;
    }
    float PBU_PROTO_0X05_Y_LONG_COORD() const {
        return _pbuProto0x05_Y_long_coord;
    }

    float PBU_PROTO_0X05_Z_HEIGHT_GEO() const {
        return _pbuProto0x05_Z_height_geo;
    }
    float PBU_PROTO_0X05_Z_HEIGHT_COORD() const {
        return _pbuProto0x05_Z_height_coord;
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
        // Базовые настройки
        this->_tcpHostIP = env.getStr("TCP_HOST_IP", "127.0.0.1").toStdString();
        this->_tcpHostPort = env.getInt("TCP_HOST_PORT", 0);
        this->_udpPort = env.getInt("UDP_PORT", 0);
        this->_ethInterfaceName =
            env.getStr("ETH_INTERFACE_NAME", "eth0").toStdString();
        this->_ethSocketBuffSize = env.getInt("ETH_SOCKET_BUF_SIZE", 2048);

        // Версии протокола
        this->_pbuProtoMajorVersion = env.getUInt8("PBU_PROTO_MAJOR_VERSION", 2);
        this->_pbuProtoMinorVersion = env.getUInt8("PBU_PROTO_MINOR_VERSION", 0);

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
    }

    /**
     * @brief Полная инициализация конфигурации
     * @param env Хэш-таблица с параметрами из .env файла
     */
    void init(const EnvLoader& env) {
        initFromEnv(env);
        initBase();
    }

private:
    AppConfig() = default; ///< Приватный конструктор

    /// @name Параметры сети
    /// @{
    std::string _tcpHostIP;         ///< IP адрес TCP сервера
                                    ///
    int _tcpHostPort;               ///< Порт TCP сервера
                                    ///
    int _udpPort;                   ///< Порт UDP
                                    ///
    std::string _ethInterfaceName;  ///< Имя сетевого интерфейса
                                    ///
    int _ethSocketBuffSize;         ///< Размер буфера MAC сокета
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

    /// @name Параметры Ethernet-пакетов
    /// @{
    uint64_t _ethProtoMacDst: 48;       ///< MAC получателя Ethernet-пакетов
    uint64_t _ethProtoMacSrc: 48;       ///< MAC отправителя Ethernet-пакетов
    uint16_t _ethProtoEtherType;        ///< Тип протокола Ethernet-пакетов
    /// @}
};
    // uint8_t _pbuProto0x05;
