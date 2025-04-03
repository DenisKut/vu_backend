#ifndef TCPMESSAGE_H
#define TCPMESSAGE_H

#pragma once
#include <cstdint>
#include <cstring>
#include <iostream>
#include <QByteArray>
#include <type_traits>

#include "utils/AppConfig.h"
#include "utils/time_provider/TimeProvider.h"
#include "libs/json/json.hpp"

using json = nlohmann::json;

/**
 * @file
 * @brief Заголовочный файл для работы с TCP-сообщениями протокола ПБУ
 */

// =========================================
// Вспомогательные функции и утилиты
// =========================================

/**
 * @brief Рассчитывает контрольную сумму для данных
 * @param data Указатель на массив данных
 * @param size Размер данных в байтах
 * @return 16-битная контрольная сумма
 */
static inline uint16_t calculateChecksum(const uint8_t* data, size_t size) {
    uint16_t checksum = 0;
    for (size_t i = 0; i < size; ++i) {
        checksum += data[i];
    }
    return checksum;
}

/**
 * @brief Преобразует enum class в базовый целочисленный тип
 * @tparam E Тип перечисления
 * @param e Значение перечисления
 * @return Базовое целочисленное значение
 */
template <typename E>
constexpr auto to_underlying(E e) -> typename std::underlying_type<E>::type {
    return static_cast<typename std::underlying_type<E>::type>(e);
}

// =========================================
// Номера сообщений по протоколу связи с ПБУ
// =========================================

/**
 * @enum PBU_MessageNumbers
 * @brief Перечисление типов сообщений протокола ПБУ
 */
enum class PBU_MessageNumbers: uint16_t {
    // -----------------------------------------
    // Сообщения от ВУ на ПБУ [ОБРАБОТКА ИНФОРМАЦИИ]
    // -----------------------------------------
    REQ_REGISTRATION    = 0x01,  ///< Запрос регистрации модуля в системе
    REQ_TIME            = 0x03,  ///< Запрос времени (пинг)
    GEO_MODULE          = 0x05,  ///< Геопозиция модуля
    MARK_TRAICE         = 0x09,  ///< Отметка трассы (Отмечает наше ВУ)
    CURRENT_STATE_RLS   = 0x1021,///< Текущие состояния РЛС-Н (1-й и 2-й отдельно)
    MARK_TARGET         = 0x1022,///< Отметка цели
    MODULE_CAPABILITIES = 0x1023,///< Возможности модуля
    FORBIDDEN_SECTORS   = 0x1024,///< Слепые зоны

    // -----------------------------------------
    // Сообщения от ПБУ на ВУ [ОБРАБОТКА ИНФОРМАЦИИ]
    // -----------------------------------------
    RES_REGISTRATION        = 0x02, ///< Подтверждение регистрации модуля
    TIMESTAMP               = 0x04, ///< Отметка времени
    GEO_COMPLEX             = 0x06, ///< Геопозиция комплекса (отправляется один раз)
    MARK_TRAICE_PBU         = 0x07, ///< Отметка трассы (от ПБУ)
    ORIENTATION_SC_COMPLEX  = 0x08, ///< Ориентация СК комплекса
    TARGET_DESIGNATION      = 0x641,///< Команда «Целеуказание»

    // -----------------------------------------
    // Сообщения от ВУ на ПБУ [КОНТРОЛЯ И УПРАВЛЕНИЯ]
    // -----------------------------------------
    MODULE_SCHEME                       = 0x20, ///< Схема модуля
    STATUS_SCHEME_ELEMENTS              = 0x21, ///< Статус элементов схемы
    VALUES_KP_STATUS_SCHEME_ELEMENTS    = 0x22, ///< Значения и статус КП элементов
    RECEIPT_CONTROL_COMMAND             = 0x23, ///< Квитанция на получение команды
    STATUS_MODULE                       = 0x24, ///< Статус Модуля
    TEXT_FOR_MESSAGE_LOG                = 0x25, ///< Текстовые сообщения в журнал
    TEXT_FOR_OPERATOR                   = 0x26, ///< Текстовые сообщения оператору
    RECEIPT_CONTROL_COMMAND_EXTENDED    = 0x27, ///< Расширенная квитанция
    VALUES_CUSTOMIZEBLE_PARAMETERS      = 0x28, ///< Настраиваемые параметры

    // -----------------------------------------
    // Сообщения от ПБУ на ВУ [КОНТРОЛЯ И УПРАВЛЕНИЯ]
    // -----------------------------------------
    COMMAND_SWITCH_ON               = 0x40, ///< Команда «Включить»
    COMMAND_SWITCH_OFF              = 0x41, ///< Команда «Выключить»
    COMMAND_RESTART                 = 0x42, ///< Команда «Перезагрузить»
    COMMAND_RESET_ALARM             = 0x43, ///< Команда «Сброс аварии»
    COMMAND_SET_CORRECTIONS         = 0x44, ///< Команда «Установить поправки»
    TEXT_FOR_OPERATOR_PBU           = 0x45, ///< Сообщения оператору от ПБУ
    REQ_MODULE_SCHEME               = 0x46, ///< Запрос схемы модуля
    COMMAND_SET_LANGUAGE            = 0x47, ///< Установка языка интерфейса
    MESSAGE_SATELLITE_NAV           = 0x48, ///< Сообщение о подавлении навигации
    COMMAND_SET_TIME                = 0x49, ///< Установка времени
    COMMAND_SET_COORDINATES_MODULE  = 0x4A, ///< Установка координат модуля
    COMMAND_SET_SETTINGS_VALUES     = 0x4B, ///< Установка параметров
};

// =========================================
// 	Общий заголовок для сообщений
// =========================================

#pragma pack(push, 1)
/**
 * @struct TcpHeader
 * @brief Базовый заголовок для всех TCP-сообщений протокола
 *
 * @details Структура содержит служебную информацию для маршрутизации
 * и проверки целостности сообщений
 */
struct TcpHeader {
    // Word - 1
    uint32_t sender: 24;    ///< Идентификатор отправителя (24 бита)
    uint32_t idxModule: 8;  ///< Номер модуля в комплексе (8 бит)

    // Word - 2
    uint8_t vMajor;         ///< Мажорная версия протокола
    uint8_t vMinor;         ///< Минорная версия протокола
    uint16_t idxPack;       ///< Сквозной номер пакета

    // Word - 3
    uint32_t sizeData: 31;  ///< Размер полезной данных (31 бит)
    uint32_t isAsku: 1;     ///< Флаг использования ПО АСКУ (1 бит)

    // Word - 4
    uint16_t typePack;      ///< Тип пакета (из PBU_MessageNumbers)
    uint16_t checkSum;      ///< Контрольная сумма заголовка

    /**
     * @brief Сериализует заголовок в массив байтов
     * @return QByteArray с бинарным представлением заголовка
     */
    QByteArray serialize() const {
        return QByteArray(reinterpret_cast<const char*>(this), sizeof(TcpHeader));
    }

    /**
     * @brief Десериализует заголовок из массива байтов
     * @param data Входные данные
     * @return Экземпляр TcpHeader
     */
    static TcpHeader deserialize(const QByteArray& data) {
        TcpHeader header;
        if(data.size() >= static_cast<int>(sizeof(TcpHeader)))
            memcpy(&header, data.constData(), sizeof(TcpHeader));
        return header;
    }

    /**
     * @brief Проверяет валидность заголовка
     * @return true если контрольная сумма совпадает
     */
    bool isValid() const {
        const size_t headerSize = sizeof(TcpHeader) - sizeof(checkSum);
        return checkSum == calculateChecksum(
                   reinterpret_cast<const uint8_t*>(this), headerSize);
    }

protected:
    /**
     * @brief Конструктор для создания заголовка
     * @param msgType Тип сообщения
     * @param dataSize Размер данных сообщения
     */
    TcpHeader(PBU_MessageNumbers msgType, uint32_t dataSize)
        : sender(AppConfig::instance().PBU_PROTO_HEAD_SENDER()),
        idxModule(AppConfig::instance().PBU_PROTO_HEAD_IDX_MODULE()),
        vMajor(AppConfig::instance().PBU_PROTO_MAJOR_VERSION()),
        vMinor(AppConfig::instance().PBU_PROTO_MINOR_VERSION()),
        idxPack(AppConfig::instance().PBU_PROTO_HEAD_IDX_PACK()),
        sizeData(dataSize),
        isAsku(AppConfig::instance().PBU_PROTO_HEAD_IS_ASKU()),
        typePack(to_underlying(msgType)),
        checkSum(0)
    {
        const auto headerSize = sizeof(TcpHeader) - sizeof(checkSum);
        checkSum = calculateChecksum(
            reinterpret_cast<const uint8_t*>(this), headerSize);
    }

    /// @brief Конструктор по умолчанию для десериализации
    TcpHeader() = default;
};
#pragma pack(pop)

static_assert(sizeof(TcpHeader) == 16, "Invalid TcpHeader size");

// =========================================
// Шаблон для создания конкретных сообщений
// =========================================

/**
 * @struct TcpMessage
 * @brief Шаблон базового класса для типизированных сообщений
 * @tparam TDerived Конкретный тип сообщения
 * @tparam MsgType Идентификатор типа сообщения
 */
template <typename TDerived, PBU_MessageNumbers MsgType>
struct TcpMessage : TcpHeader {
    static constexpr PBU_MessageNumbers MESSAGE_TYPE = MsgType; ///< Тип сообщения

    /// @brief Конструктор инициализирует заголовок
    TcpMessage() : TcpHeader(MsgType, sizeof(TDerived) - sizeof(TcpHeader)) {}

    /**
     * @brief Сериализует полное сообщение
     * @return QByteArray с бинарным представлением
     */
    QByteArray serializeFull() const {
        QByteArray data(sizeof(TDerived), 0);
        memcpy(data.data(), this, sizeof(TDerived));
        return data;
    }

    /**
     * @brief Десериализует сообщение из данных
     * @param data Входные данные
     * @return Экземпляр сообщения
     */
    static TDerived deserialize(const QByteArray& data) {
        TDerived message;
        if (data.size() >= static_cast<int>(sizeof(TDerived))) {
            memcpy(&message, data.constData(), sizeof(TDerived));
        }
        return message;
    }
};

// =========================================
// Конкретные реализации сообщений
// =========================================

#pragma pack(push, 1)
/**
 * @struct TcpMessage_0x01
 * @brief Сообщение запроса регистрации (0x01)
 */
struct TcpMessage_0x01 : TcpMessage<TcpMessage_0x01, PBU_MessageNumbers::REQ_REGISTRATION> {
    uint32_t idManuf: 8;        ///< Идентификатор производителя (8 бит)
    uint32_t serialNum: 24;     ///< Серийный номер (24 бита)

    uint8_t versHardMaj;        ///< Мажорная версия аппаратной части
    uint8_t versHardMin;        ///< Минорная версия аппаратной части
    uint8_t versProgMaj: 7;     ///< Мажорная версия ПО (7 бит)
    uint8_t isInfo: 1;          ///< Флаг обработки информации (1 бит)
    uint8_t versProgMin: 7;     ///< Минорная версия ПО (7 бит)
    uint8_t isControl: 1;       ///< Флаг управления (1 бит)

    /// @brief Инициализирует поля из конфигурации
    TcpMessage_0x01() {
        idManuf = AppConfig::instance().PBU_PROTO_0X01_ID_MANUF();
        serialNum = AppConfig::instance().PBU_PROTO_0X01_SERIAL_NUM();
        versHardMaj = AppConfig::instance().PBU_PROTO_0X01_VERS_HARD_MAJ();
        versHardMin = AppConfig::instance().PBU_PROTO_0X01_VERS_HARD_MIN();
        versProgMaj = AppConfig::instance().PBU_PROTO_0X01_VERS_PROG_MAJ();
        isInfo = AppConfig::instance().PBU_PROTO_0X01_IS_INFO();
        versProgMin = AppConfig::instance().PBU_PROTO_0X01_VERS_PROG_MIN();
        isControl = AppConfig::instance().PBU_PROTO_0X01_IS_CONTROL();
    }
};
#pragma pack(pop)

static_assert(sizeof(TcpMessage_0x01) == 24, "Invalid size [TcpMessage_0x01]");

#pragma pack(push, 1)
/**
 * @struct TcpMessage_0x03
 * @brief Сообщение запроса времени (0x03)
 */
struct TcpMessage_0x03 : TcpMessage<TcpMessage_0x03, PBU_MessageNumbers::REQ_TIME> {
    uint32_t timeRequest1; ///< Младшие 32 бита временной метки
    uint32_t timeRequest2; ///< Старшие 32 бита временной метки

    /// @brief Инициализирует временную метку текущим временем
    TcpMessage_0x03() {
        uint64_t time = to_uint64(TimeProvider::instance().getMs());
        timeRequest1 = to_uint32(time & 0xFFFFFFFF);
        timeRequest2 = to_uint32(time >> 32);
    }
};
#pragma pack(pop)

static_assert(sizeof(TcpMessage_0x03) == 24, "Invalid size [TcpMessage_0x03]");

#pragma pack(push, 1)
/**
 * @struct TcpMessage_0x05
 * @brief Сообщение геопозиции модуля (0x05)
 */
struct TcpMessage_0x05 : TcpMessage<TcpMessage_0x05, PBU_MessageNumbers::GEO_MODULE> {
    uint32_t typeData: 3; ///< Тип данных (1 - гео, 2 - координаты)
    uint32_t isValid: 1;  ///< Флаг достоверности данных
    uint32_t reserve: 28; ///< Резервные биты

    float X_lat;  ///< Широта/координата X
    float Y_long; ///< Долгота/координата Y
    float Z_height; ///< Высота/координата Z

    /// @brief Инициализирует данные из конфигурации
    TcpMessage_0x05() {
        typeData = AppConfig::instance().PBU_PROTO_0X05_TYPE_DATA();
        isValid = AppConfig::instance().PBU_PROTO_0X05_IS_VALID();
        if (AppConfig::instance().PBU_PROTO_0X05_TYPE_DATA() ==
            to_uint8(PBU_0x05_TypeData::GEOPOSITION)) {
            X_lat = AppConfig::instance().PBU_PROTO_0X05_X_LAT_GEO();
            Y_long = AppConfig::instance().PBU_PROTO_0X05_Y_LONG_GEO();
            Z_height = AppConfig::instance().PBU_PROTO_0X05_Z_HEIGHT_GEO();
        } else {
            X_lat = AppConfig::instance().PBU_PROTO_0X05_X_LAT_COORD();
            Y_long = AppConfig::instance().PBU_PROTO_0X05_Y_LONG_COORD();
            Z_height = AppConfig::instance().PBU_PROTO_0X05_Z_HEIGHT_COORD();
        }
    }
};
#pragma pack(pop)

static_assert(sizeof(TcpMessage_0x05) == 32, "Invalid size [TcpMessage_0x05]");

/**
 * @class MessageFactory
 * @brief Фабрика для создания и парсинга сообщений
 */
class MessageFactory {
public:
    /**
     * @brief Создает сообщение указанного типа
     * @tparam T Тип сообщения (должен наследовать от TcpHeader)
     * @return Экземпляр сообщения
     */
    template<typename T>
    static typename std::enable_if<std::is_base_of<TcpHeader, T>::value, T>::type create() {
        return T();
    }

    /**
     * @brief Парсит входящие данные в сообщение
     * @param data Входные данные
     * @return Указатель на сообщение или nullptr при ошибке
     */
    static std::unique_ptr<TcpHeader> parse(const QByteArray& data) {
        if (data.size() < static_cast<int>(sizeof(TcpHeader))) {
            return nullptr;
        }

        /// @brief Десериализуем базовый заголовок
        TcpHeader header = TcpHeader::deserialize(data);
        if (!header.isValid()) {
            return nullptr;
        }

        /// @brief Определяем тип сообщения
        switch (static_cast<PBU_MessageNumbers>(header.typePack)) {
        case PBU_MessageNumbers::REQ_REGISTRATION: {
            return parseMessage<TcpMessage_0x01>(data);
        }
        // Обработка других типов...
        default: return nullptr;
        }
    }

private:
    template<typename T>
    static std::unique_ptr<TcpHeader> parseMessage(const QByteArray& data) {
        if(data.size() < static_cast<int>(sizeof(T))) return nullptr;

        T msg = T::deserialize(data);
        if(!msg.isValid()) return nullptr;

        return std::unique_ptr<TcpHeader>(new T(msg));
    }
};

///////////////////////////////////////////////////////
/// Пример работы с сообщениями
///////////////////////////////////////////////////////
///
/// // Создание сообщения
/// auto msg = MessageFactory::create<TcpMessage_0x01>();
/// QByteArray data = msg.serializeFull();
///
/// // Парсинг
/// auto parsed = MessageFactory::parse(data);
/// if(auto* msg01 = dynamic_cast<TcpMessage_0x01*>(parsed.get())) {
///    // Работа с сообщением
/// }
///////////////////////////////////////////////////////

#endif // TCPMESSAGE_H
