#ifndef TCPMESSAGE_H
#define TCPMESSAGE_H

#pragma once
#include <cstdint>
#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <QByteArray>
#include <QDateTime>
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
    REQ_REGISTRATION    = 0x01,     ///< Запрос регистрации модуля в системе
                                    ///
    REQ_TIME            = 0x03,     ///< Запрос времени (пинг)
                                    ///
    GEO_MODULE          = 0x05,     ///< Геопозиция модуля
                                    ///
    MARK_TRAICE         = 0x09,     ///< Отметка трассы (Отмечает наше ВУ)
                                    ///
    CURRENT_STATE_RLS   = 0x1021,   ///< Текущие состояния РЛС-Н (1-й и 2-й
                                    ///< отдельно)
                                    ///
    MARK_TARGET         = 0x1022,   ///< Отметка цели
                                    ///
    MODULE_CAPABILITIES = 0x1023,   ///< Возможности модуля
                                    ///
    FORBIDDEN_SECTORS   = 0x1024,   ///< Слепые зоны

    // -----------------------------------------
    // Сообщения от ПБУ на ВУ [ОБРАБОТКА ИНФОРМАЦИИ]
    // -----------------------------------------
    RES_REGISTRATION        = 0x02, ///< Подтверждение регистрации модуля
                                    ///
    TIMESTAMP               = 0x04, ///< Отметка времени
                                    ///
    GEO_COMPLEX             = 0x06, ///< Геопозиция комплекса (отправляется один
                                    ///< раз)
                                    ///
    MARK_TRAICE_PBU         = 0x07, ///< Отметка трассы (от ПБУ)
                                    ///
    ORIENTATION_SC_COMPLEX  = 0x08, ///< Ориентация СК комплекса
                                    ///
    TARGET_DESIGNATION      = 0x641,///< Команда «Целеуказание»

    // -----------------------------------------
    // Сообщения от ВУ на ПБУ [КОНТРОЛЯ И УПРАВЛЕНИЯ]
    // -----------------------------------------
    MODULE_SCHEME                       = 0x20, ///< Схема модуля
                                                ///
    STATUS_SCHEME_ELEMENTS              = 0x21, ///< Статус элементов схемы
                                                ///
    VALUES_KP_STATUS_SCHEME_ELEMENTS    = 0x22, ///< Значения и статус КП
                                                ///< элементов
                                                ///
    RECEIPT_CONTROL_COMMAND             = 0x23, ///< Квитанция на получение
                                                ///< команды
                                                ///
    STATUS_MODULE                       = 0x24, ///< Статус Модуля
                                                ///
    TEXT_FOR_MESSAGE_LOG                = 0x25, ///< Текстовые сообщения в
                                                ///< журнал
                                                ///
    TEXT_FOR_OPERATOR                   = 0x26, ///< Текстовые сообщения
                                                ///< оператору
                                                ///
    RECEIPT_CONTROL_COMMAND_EXTENDED    = 0x27, ///< Расширенная квитанция
                                                ///
    VALUES_CUSTOMIZEBLE_PARAMETERS      = 0x28, ///< Настраиваемые параметры

    // -----------------------------------------
    // Сообщения от ПБУ на ВУ [КОНТРОЛЯ И УПРАВЛЕНИЯ]
    // -----------------------------------------
    COMMAND_SWITCH_ON               = 0x40, ///< Команда «Включить»
                                            ///
    COMMAND_SWITCH_OFF              = 0x41, ///< Команда «Выключить»
                                            ///
    COMMAND_RESTART                 = 0x42, ///< Команда «Перезагрузить»
                                            ///
    COMMAND_RESET_ALARM             = 0x43, ///< Команда «Сброс аварии»
                                            ///
    COMMAND_SET_CORRECTIONS         = 0x44, ///< Команда «Установить поправки»
                                            ///
    TEXT_FOR_OPERATOR_PBU           = 0x45, ///< Сообщения оператору от ПБУ
                                            ///
    REQ_MODULE_SCHEME               = 0x46, ///< Запрос схемы модуля
                                            ///
    COMMAND_SET_LANGUAGE            = 0x47, ///< Установка языка интерфейса
                                            ///
    MESSAGE_SATELLITE_NAV           = 0x48, ///< Сообщение о подавлении
                                            ///< навигации
                                            ///
    COMMAND_SET_TIME                = 0x49, ///< Установка времени
                                            ///
    COMMAND_SET_COORDINATES_MODULE  = 0x4A, ///< Установка координат модуля
                                            ///
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
        return QByteArray(reinterpret_cast<const char*>(this),
                          sizeof(TcpHeader));
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

    std::string toString() const {
        std::ostringstream oss;
        oss << "TcpHeader:\n"
            << "  Sender: " << sender << "\n"
            << "  Module Index: " << idxModule << "\n"
            << "  Version: " << static_cast<int>(vMajor) << "."
            << static_cast<int>(vMinor) << "\n"
            << "  Packet Index: " << idxPack << "\n"
            << "  Data Size: " << sizeData << " bytes\n"
            << "  ASKU Flag: " << (isAsku ? "Yes" : "No") << "\n"
            << "  Message Type: 0x" << std::hex << typePack << std::dec << "\n"
            << "  Checksum: 0x" << std::hex << checkSum << std::dec
            << (isValid() ? " (Valid)" : " (Invalid)");
        return oss.str();
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
    /// Тип сообщения
    static constexpr PBU_MessageNumbers MESSAGE_TYPE = MsgType;

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
// -----------------------------------------
// Сообщения от ВУ на ПБУ [ОБРАБОТКА ИНФОРМАЦИИ]
// -----------------------------------------
#pragma pack(push, 1)
/**
 * @struct TcpMessage_0x01
 * @brief Сообщение запроса регистрации (0x01)
 */
struct TcpMessage_0x01 : TcpMessage<TcpMessage_0x01,
                                    PBU_MessageNumbers::REQ_REGISTRATION> {
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

    std::string toString() const {
        std::ostringstream oss;
        oss << TcpHeader::toString() << "\n"
            << "Registration Request (0x01):\n"
            << "  Manufacturer ID: 0x" << std::hex << static_cast<int>(idManuf)
            << std::dec << "\n"
            << "  Serial Number: " << serialNum << "\n"
            << "  Hardware Version: " << static_cast<int>(versHardMaj) << "."
            << static_cast<int>(versHardMin) << "\n"
            << "  Software Version: " << static_cast<int>(versProgMaj & 0x7F) << "."
            << static_cast<int>(versProgMin & 0x7F) << "\n"
            << "  Info Processing: " << (isInfo ? "Yes" : "No") << "\n"
            << "  Control Capability: " << (isControl ? "Yes" : "No");
        return oss.str();
    }
};
#pragma pack(pop)

static_assert(sizeof(TcpMessage_0x01) == 16 + 8, "Invalid size [TcpMessage_0x01]");

#pragma pack(push, 1)
/**
 * @struct TcpMessage_0x03
 * @brief Сообщение запроса времени (0x03)
 */
struct TcpMessage_0x03 : TcpMessage<TcpMessage_0x03,
                                    PBU_MessageNumbers::REQ_TIME> {
    uint32_t timeRequest1; ///< Младшие 32 бита временной метки
    uint32_t timeRequest2; ///< Старшие 32 бита временной метки

    /// @brief Инициализирует временную метку текущим временем
    TcpMessage_0x03() {
        uint64_t time = TimeProvider::instance().getTimeForRequest();
        timeRequest1 = to_uint32(time & 0xFFFFFFFF);
        timeRequest2 = to_uint32(time >> 32);
    }

    std::string toString() const {
        std::ostringstream oss;
        uint64_t time = (static_cast<uint64_t>(timeRequest2) << 32) | timeRequest1;
        oss << TcpHeader::toString() << "\n"
            << "Time Request (0x03):\n"
            << "  Request Time: " << time << " ms since epoch\n"
            << "  Human Readable: " << TimeProvider::instance()
                                                        .msToDateTime(time);
        return oss.str();;
    }
};
#pragma pack(pop)

static_assert(sizeof(TcpMessage_0x03) == 16 + 8, "Invalid size [TcpMessage_0x03]");

#pragma pack(push, 1)
/**
 * @struct TcpMessage_0x05
 * @brief Сообщение геопозиции модуля (0x05)
 */
struct TcpMessage_0x05 : TcpMessage<TcpMessage_0x05,
                                    PBU_MessageNumbers::GEO_MODULE> {
    uint32_t typeData: 3;   ///< Тип данных (1 - гео, 2 - координаты)
    uint32_t isValid: 1;    ///< Флаг достоверности данных
    uint32_t reserve: 28;   ///< Резервные биты

    float X_lat;            ///< Широта/координата X
    float Y_long;           ///< Долгота/координата Y
    float Z_height;         ///< Высота/координата Z

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

    std::string toString() const {
        std::ostringstream oss;
        oss << TcpHeader::toString() << "\n"
            << "Module Geoposition (0x05):\n"
            << "  Data Type: " << (typeData == 1 ? "Geographic" : "Coordinates") << "\n"
            << "  Data Valid: " << (isValid ? "Yes" : "No") << "\n";

        if (typeData == 1) {
            oss << "  Latitude: " << X_lat << "°\n"
                << "  Longitude: " << Y_long << "°\n"
                << "  Height: " << Z_height << " m";
        } else {
            oss << "  X Coordinate: " << X_lat << " m\n"
                << "  Y Coordinate: " << Y_long << " m\n"
                << "  Z Coordinate: " << Z_height << " m";
        }
        return oss.str();
    }
};
#pragma pack(pop)

static_assert(sizeof(TcpMessage_0x05) == 16 + 16, "Invalid size [TcpMessage_0x05]");

#pragma pack(push, 1)
/**
 * @struct TcpMessage_0x1021
 * @brief Сообщение текущего состояния РЛМН
 * @details Данное сообщение отправляется при смене любого передаваемого
 * параметра (делаем только раз в секунду по каждой РЛМН), а также раз в секунду
 * по каждому независимому радиолокационному каналу (РЛ‑канал), входящему в
 * состав ВУ
 */
struct TcpMessage_0x1021 : TcpMessage<TcpMessage_0x1021,
                                      PBU_MessageNumbers::CURRENT_STATE_RLS> {
    uint32_t time1;     ///< Младшие 32 бита временной метки
    uint32_t time2;     ///< Старшие 32 бита временной метки
    uint8_t numDrive;   ///< Условный номер РЛ-канала в составе модуля (8 бит)
    uint8_t isRemote;   ///< Признак внешнего управления (8 бит)
    uint16_t reserve;   ///< Резервное поле (16 бит - не используется)
    float azCur;        ///< Текущее положение по азимуту (32 бит)
    float elCur;        ///< Текущее положение по углу места (32 бит)
    float dAzVisible;   ///< Ширина зоны обзора/луча по азимуту (32 бит)
    float dElVisible;   ///< Ширина поля обзора/луча по углу места (32 бит)

    TcpMessage_0x1021(AppConfig::ModulePPU_num num_device)
        : numDrive(to_uint8(num_device)),
        isRemote(AppConfig::instance().getModulePPU_isRemote(num_device)),
        reserve(0),
        azCur(AppConfig::instance().getModulePPU_az(num_device)),
        elCur(AppConfig::instance().getModulePPU_an(num_device)),
        dAzVisible(3),
        dElVisible(3) {
        uint64_t time = to_uint64(TimeProvider::instance().getMs());
        time1 = to_uint32(time & 0xFFFFFFFF);
        time2 = to_uint32(time >> 32);

        /// @warning !!! Для заполнения требует наличия заведённых структур для
        /// объектов и их траекторий
    }

    std::string toString() const {
        std::ostringstream oss;
        uint64_t time = (static_cast<uint64_t>(time2) << 32) | time1;
        oss << TcpHeader::toString() << "\n"
            << "RLS Current State (0x1021):\n"
            << "  Time: " << TimeProvider::instance().msToDateTime(time) << "\n"
            << "  RLS Channel: " << static_cast<int>(numDrive) << "\n"
            << "  Remote Control: " << (isRemote ? "Yes" : "No") << "\n"
            << "  Current Azimuth: " << azCur << "°\n"
            << "  Current Elevation: " << elCur << "°\n"
            << "  Azimuth Visible Zone: " << dAzVisible << "°\n"
            << "  Elevation Visible Zone: " << dElVisible << "°";
        return oss.str();
    }
};
#pragma pack(pop)

static_assert(sizeof(TcpMessage_0x1021) == 16 + 28,
              "Invalid size [TcpMessage_0x1021]");

#pragma pack(push, 1)
/**
 * @struct TcpMessage_0x1022
 * @brief Сообщение отметки цели
 * @details Данное сообщение выдается по каждой обнаруженной в ВУ отметке
 * цели
 */
struct TcpMessage_0x1022 : TcpMessage<TcpMessage_0x1022,
                                      PBU_MessageNumbers::MARK_TARGET> {
    uint8_t trackStatus: 2;     ///< @brief Статус трассы
                                ///< @details Возможные значения:
                                ///< - «0»: Передается в случае продолжения
                                ///< сопровождения трассы;
                                ///< - «1»: Передается в случае выдачи первой
                                ///< отметки вновь обнаруженной трассы;
                                ///< - «2»: Передается в случае сброса
                                ///< сопровождения трассы (в этом случае
                                ///< остальные поля сообщения, кроме Номер
                                ///< трассы, игнорируются).

    uint8_t isExtra: 1;         ///< @brief Признак пропуска обнаружения
                                ///< @details Возможные значения:
                                ///< - **0**: передается в случае обнаружения
                                ///< отметки в текущем запланированном по трассе
                                ///< зондировании;
                                ///< - **1**: передается в случае \e отсутствия
                                ///< обнаружения в текущем запланированном по
                                ///< трассе зондировании. В этом случае в полях:
                                ///<   - `<span style="color:#FF5733">Азимут
                                ///< отметки</span>`
                                ///<   - `<span style="color:#FF5733">Дальность
                                ///< отметки</span>`
                                ///<   - `<span style="color:#FF5733">Угол места
                                ///< отметки</span>`
                                ///< передаются <b>экстраполированные</b>
                                ///< значения координат

    uint8_t isTest: 1;          ///< @brief Признак тестовой отметки
                                ///< @details Возможные значения:
                                ///< - **0**: отметка не тестовая;
                                ///< - **1**: отметка тестовая.

    uint8_t numDrive: 2;        ///< @brief Условный номер РЛ-канала в модуле
                                ///< @details Передается условный номер
                                ///< РЛ-канала которым обнаружена
                                ///< (сопровождается) трасса.

    uint8_t reserve_1: 2;       ///< @brief Резервное поле (не используется)

    uint8_t typeTrack;          ///< @brief Тип трассы
                                ///< @details Передается тип, которому
                                ///< соответствует сопровождаемая трасса, поле
                                ///< может содержать только следующие значения:
                                ///< - «1»: низко-скоростной БПЛА;
                                ///< - «2»: скоростной БПЛА или легкомоторный
                                ///< самолёт;
                                ///< - «3»: крылатая ракета;
                                ///< - «4»: «птица» (группа птиц);
                                ///< - «5»: крупноразмерный самолёт;
                                ///< - «6»: вертолет;
                                ///< - «7»: прочее (тип не определен).

    uint16_t idxCeil;           ///< @brief Порядковый индекс отметки
                                ///< @details сквозной порядковый номер отметки
                                ///< в ПО РЛС-Н, инкрементируется для каждой
                                ///< новой отметки
                                ///< @warning допустимые значения - `0…65535`.

    float azimuth;              ///< @brief Азимут отметки
                                ///< @details Передается оценка азимута отметки
                                ///< в градусах относительно направления на
                                ///< географический север относительно точки
                                ///< стояния РЛС-Н.
                                ///< @warning Допустимые значения `от 0 до
                                ///< 360 градусов` **включительно**.

    float range;                ///< @brief Дальность отметки
                                ///< @details Передается оценка наклонной
                                ///< дальности отметки в метрах относительно
                                ///< точки стояния РЛС-Н.
                                ///< @warning Отрицательные значения
                                ///< используются как признак недостоверных
                                ///< данных.

    float eps;                  ///< @brief Угол места
                                ///< @details Передается оценка угла места
                                ///< отметки в градусах относительно точки
                                ///< стояния РЛС-Н.
                                ///< @warning Возможные значения `от -90 до 90
                                ///< градусов` **включительно**.

    float vel;                  ///< @brief Оценка радиальной скорости цели
                                ///< @details В данном поле выдается
                                ///< рассчитанное значение радиальной скорости
                                ///< цели.
                                ///< @warning Допустимые значения радиальной
                                ///< скорости `-1300…+1300` **включительно**.
                                ///< \e Значения вне допустимого интервала
                                ///< используются как признак недостоверной
                                ///< оценки.

    float velAz;                ///< @brief Скорость изменения азимута объекта
                                ///< в градусах/с
                                ///< @details В данном поле выдается
                                ///< рассчитанное значение скорости изменения
                                ///< азимута цели в град/с.

    float velEps;               ///< @brief	Скорость изменения угла места
                                ///< объекта в градусах/с
                                ///< @details В данном поле выдается
                                ///< рассчитанное значение скорости изменения
                                ///< угла места цели в град/с.

    /// @name UTC
    /// @details передается в виде 64-битного значения кол-ва миллисекунд,
    /// прошедших с 00:00:00.000 1970 01 01 (Unix Epoch) на момент обнаружения
    /// отметки.
    /// @{
    uint32_t time1;             ///< @brief Младшие 32 бита временной метки

    uint32_t time2;             ///< @brief Старшие 32 бита временной метки
    /// @}

    float snrSum;               ///< @brief Оценка ОСШ
                                ///< @details Передается отношение сигнал/шум на
                                ///< выходе СДЦ в дБ.
                                ///< @warning Допустимые значения ОСШ:
                                ///< `-30…+130` **включительно**. Значения вне
                                ///< этого интервала используются как признак
                                ///< \e недостоверной оценки.

    float epr;                  ///< @brief Оценка ЭПР цели
                                ///< @details Передается оценка ЭПР отметки с
                                ///< учетом расчетного значения потенциала
                                ///< станции в м^2.
                                ///< @warning Допустимые значения ЭПР 0,0…150.
                                ///< Отрицательные значения используются как
                                ///< признак недостоверной оценки.

    float maxRange;             ///< @brief Интервал однозначного определения
                                ///< дальности
                                ///< @details Передается интервал однозначного
                                ///< определения наклонной дальности в метрах.

    uint16_t numbTrack;         ///< @brief Номер трассы
                                ///< @details В данном поле передается
                                ///< порядковый номер трассы
                                ///< @warning Допустимые значения `1…65535`.

    uint16_t numbTrackServr;	///< @brief Номер трассы от сервера
                                ///< @details В данном поле передается
                                ///< порядковый номер трассы ПО Сервера, которой
                                ///< соответствует передаваемая информация.
                                ///< @warning Порядковый номер трассы ПО Сервера
                                ///< передается в команде «Целеуказание» (0x641)

    float sigAz;                ///< @brief	Оценка погрешности измерения азимута
                                ///< @details В данном поле передается оценка
                                ///< погрешности определения азимута в угловых
                                ///< минутах.

    float sigEps;               ///< @brief	Оценка погрешности измерения УМ
                                ///< @details В данном поле передается оценка
                                ///< погрешности определения угла места в
                                ///< угловых минутах.

    float sigD;                 ///< @brief	Оценка погрешности измерения
                                ///< дальности
                                ///< @details В данном поле передается оценка
                                ///< погрешности определения дальности в метрах.


    float sigVr;                ///< @brief	Оценка погрешности измерения
                                ///< радиальной скорости
                                ///< @details В данном поле передается оценка
                                ///< погрешности определения радиальной скорости
                                ///< в м/с.

    uint32_t rangeCnt: 6;       ///< @brief Количество отсчетов дальности ДСП
                                ///< @details В данное поле записывается
                                ///< количество передаваемых отсчетов дальности
                                ///< дальностно-скоростного портрета (ДСП).
                                ///< @warning
                                ///< @{
                                ///< @p При отсутствии дальностно-скоростного
                                ///< портрета (при сбросе трассы) в данном поле
                                ///< передается **ноль**.
                                ///< @p При передаче пропуска сопровождения ДСП
                                ///< формируется в районе экстраполированного
                                ///< положения цели.
                                ///< @}

    uint32_t speedCnt : 12;     ///< @brief Количество отсчетов радиальной
                                ///< скорости ДСП
                                ///< @details В данное поле записывается
                                ///< количество передаваемых отсчетов радиальной
                                ///< скорости дальностно-скоростного портрета.
                                ///< @warning При отсутствии дальностно-
                                ///< скоростного портрета (при сбросе) в данном
                                ///< поле передается **ноль**.

    uint32_t chanCnt : 3;       ///< @brief Количество передаваемых каналов ДСП
                                ///< **(у нас только один канал - суммарный)**
                                ///< @details В данное поле записывается
                                ///< количество каналов, для которых передаются
                                ///< данные ДСП.
                                ///< @warning Возможные значения от 1 до 4
                                ///< **включительно**.
                                ///< \e ОДНАКО у нас ОДИН КАНАЛ

    uint32_t reserve_2 : 11;    ///< @brief Резервное поле (не используется)

    float rangeStart;           ///< @brief Минимальная дальность
                                ///< @details В данном поле передается
                                ///< минимальная дальность (дальность первого
                                ///< отсчета) дальностно-скоростного портрета в
                                ///< метрах.

    float rangeStep;            ///< @brief Шаг по дальности
                                ///< @details В данном поле передается размер
                                ///< одного отсчета по дальности передаваемого
                                ///< дальностно-скоростного портрета в метрах.

    float vradStart;            ///< @brief Минимальная радиальная скорость
                                ///< @details В данном поле передается
                                ///< минимальная радиальная скорость
                                ///< передаваемого дальностно-скоростного
                                ///< портрета в м/с.

    float vradStep;             ///< @brief Шаг по радиальной скорости
                                ///< @details В данном поле передается размер
                                ///< одного отсчета по радиальной скорости
                                ///< передаваемого дальностно-скоростного
                                ///< портрета в м/с.


    /// @brief Массив мощностей ДСП [rangeCnt][speedCnt]
    /// @details Динамические данные (Массив \e по протоколу - **pow**)
    float* powRangeVel = nullptr;
    // float** powRangeVel = nullptr;

    TcpMessage_0x1022(float* powData = nullptr, uint8_t rangeBins = 0,
                      uint8_t speedBins = 0)
        : rangeCnt(rangeBins), speedCnt(speedBins) {
        uint64_t time = to_uint64(TimeProvider::instance().getMs());
        time1 = to_uint32(time & 0xFFFFFFFF);
        time2 = to_uint32(time >> 32);

        chanCnt = 1; // Фиксированное значение

        if (powData && rangeCnt > 0 && speedCnt > 0) {
            allocatePowRangeVel();
            copyPowData(powData);
        }

        updateDataSize();
    }

    // TcpMessage_0x1022(
    //     float** powData
    //     /* Если необходимо, доп. параметры */
    //     ) {
    //     uint64_t time = to_uint64(TimeProvider::instance().getMs());
    //     time1 = to_uint32(time & 0xFFFFFFFF);
    //     time2 = to_uint32(time >> 32);

    //     chanCnt = 1; // Фиксированное значение (У нас один канал - суммарный)

    //     /// @warning !!! Для заполнения требует наличия заведённых структур для
    //     /// объектов и их траекторий

    //     // Выделение памяти и копирование данных
    //     if (powData && rangeCnt > 0 && speedCnt > 0) {
    //         allocatePowRangeVel();
    //         copyPowData(powData);
    //     }

    //     // Обновляем размер данных в заголовке
    //     updateDataSize();
    // }

    /// @brief Деструктор для очистки памяти
    ~TcpMessage_0x1022() {
        freePowRangeVel();
    }

    /**
     * @brief Выделяет память под массив мощностей
     * @throws std::bad_alloc При ошибке выделения памяти
     */
    void allocatePowRangeVel() {
        freePowRangeVel();

        if (rangeCnt > 0 && speedCnt > 0) {
            powRangeVel = new float[rangeCnt * speedCnt]();
        }
    }
    // void allocatePowRangeVel() {
    //     freePowRangeVel();

    //     if (rangeCnt > 0 && speedCnt > 0) {
    //         powRangeVel = new float*[rangeCnt];
    //         for (uint8_t i = 0; i < rangeCnt; ++i) {
    //             powRangeVel[i] = new float[speedCnt]{};
    //         }
    //     }
    // }

    /**
     * @brief Копирует внешние данные в массив мощностей
     * @param src Исходный массив данных [rangeCnt][speedCnt]
     */
    void copyPowData(const float* src) {
        if (!src || !powRangeVel) return;

        std::copy(src, src + rangeCnt * speedCnt, powRangeVel);
    }
    // void copyPowData(float** src) {
    //     if (!src || !powRangeVel) return;

    //     for (uint8_t i = 0; i < rangeCnt; ++i) {
    //         std::copy(src[i], src[i] + speedCnt, powRangeVel[i]);
    //     }
    // }

    /**
     * @brief Обновляет размер данных в заголовке
     */
    void updateDataSize() {
        size_t dataSize = sizeof(TcpMessage_0x1022) - sizeof(TcpHeader);
        if (powRangeVel) {
            dataSize += rangeCnt * speedCnt * sizeof(float);
        }
        sizeData = static_cast<uint32_t>(dataSize) - sizeof(float*);

        // Обновляем контрольную сумму заголовка
        const size_t headerSize = sizeof(TcpHeader) - sizeof(checkSum);
        checkSum = calculateChecksum(reinterpret_cast<const uint8_t*>(this),
                                     headerSize);
    }
    // void updateDataSize() {
    //     size_t dynamicSize = 0;
    //     if (powRangeVel && rangeCnt > 0 && speedCnt > 0) {
    //         dynamicSize = rangeCnt * speedCnt * sizeof(float);
    //     }
    //     sizeData = sizeof(TcpMessage_0x1022) - sizeof(TcpHeader) + dynamicSize;
    // }

    /// @brief Полная сериализация сообщения
    /// @warning Переопределяем метод сереализации для этого сообщения, т.к.
    /// наличие динамического массива вносит коррективы
    QByteArray serializeFull() const {
        // Фиксированная часть
        QByteArray data(sizeof(TcpMessage_0x1022), 0);
        memcpy(data.data(), this, sizeof(TcpMessage_0x1022));

        // Динамическая часть (данные ДСП)
        if (powRangeVel && rangeCnt > 0 && speedCnt > 0) {
            data.append(reinterpret_cast<const char*>(powRangeVel),
                        rangeCnt * speedCnt * sizeof(float));
        }

        return data;
    }
    // QByteArray serializeFull() const {
    //     // Фиксированная часть
    //     QByteArray data(sizeof(TcpMessage_0x1022), 0);
    //     memcpy(data.data(), this, sizeof(TcpMessage_0x1022));

    //     // Динамическая часть
    //     if (powRangeVel && rangeCnt > 0 && speedCnt > 0) {
    //         for (uint8_t i = 0; i < rangeCnt; ++i) {
    //             data.append(reinterpret_cast<const char*>(powRangeVel[i]),
    //                         speedCnt * sizeof(float));
    //         }
    //     }

    //     return data;
    // }

    std::string toString() const {
        std::ostringstream oss;
        uint64_t time = (static_cast<uint64_t>(time2) << 32) | time1;
        oss << TcpHeader::toString() << "\n"
            << "Target Mark (0x1022):\n"
            << "  Detection Time: " << TimeProvider::instance()
                                           .msToDateTime(time) << "\n"
            << "  Track Status: " << static_cast<int>(trackStatus) << "\n"
            << "  Is Extra: " << (isExtra ? "Yes" : "No") << "\n"
            << "  Is Test: " << (isTest ? "Yes" : "No") << "\n"
            << "  RLS Channel: " << static_cast<int>(numDrive) << "\n"
            << "  Track Type: " << static_cast<int>(typeTrack) << "\n"
            << "  Track Index: " << idxCeil << "\n"
            << "  Azimuth: " << azimuth << "°\n"
            << "  Range: " << range << " m\n"
            << "  Elevation: " << eps << "°\n"
            << "  Radial Velocity: " << vel << " m/s\n"
            << "  Azimuth Velocity: " << velAz << "°/s\n"
            << "  Elevation Velocity: " << velEps << "°/s\n"
            << "  SNR: " << snrSum << " dB\n"
            << "  RCS: " << epr << " m²\n"
            << "  Max Range: " << maxRange << " m\n"
            << "  Track Number: " << numbTrack << "\n"
            << "  Server Track Number: " << numbTrackServr << "\n"
            << "  Azimuth Error: " << sigAz << "'\n"
            << "  Elevation Error: " << sigEps << "'\n"
            << "  Range Error: " << sigD << " m\n"
            << "  Velocity Error: " << sigVr << " m/s\n"
            << "  DSP Parameters:\n"
            << "    Range Bins: " << rangeCnt << "\n"
            << "    Speed Bins: " << speedCnt << "\n"
            << "    Channels: " << static_cast<int>(chanCnt) << "\n"
            << "    Range Start: " << rangeStart << " m\n"
            << "    Range Step: " << rangeStep << " m\n"
            << "    Velocity Start: " << vradStart << " m/s\n"
            << "    Velocity Step: " << vradStep << " m/s";
        return oss.str();
    }

private:

    /// @brief Освобождает память массива мощностей
    void freePowRangeVel() {
        delete[] powRangeVel;
        powRangeVel = nullptr;
    }
    // void freePowRangeVel() {
    //     if (powRangeVel) {
    //         for (uint8_t i = 0; i < rangeCnt; ++i) {
    //             delete[] powRangeVel[i];
    //         }
    //         delete[] powRangeVel;
    //         powRangeVel = nullptr;
    //     }
    // }
};
#pragma pack(pop)

static_assert(sizeof(TcpMessage_0x1022) == 16 + 22*4 + sizeof(float*) ,
              "Invalid size [TcpMessage_0x1022]");

#pragma pack(push, 1)
/**
 * @struct TcpMessage_0x1023
 * @brief Сообщение о возможностях модуля с JSON-данными
 * @details Данное сообщение отправляется сразу после регистрации модуля в
 * системе.
 * @warning До получения данного сообщения со стороны ПО СОИ **НЕ МОЖЕТ**
 * поступать никаких команд управления модулем.
 */
struct TcpMessage_0x1023 : TcpMessage<TcpMessage_0x1023,
                                      PBU_MessageNumbers::MODULE_CAPABILITIES> {
    char* jsonData;     ///< @brief Указатель на сырые JSON-данные
                        ///< @note ПРИМЕЧАНИЕ:
                        ///< - 1: Название ключей в JSON должно быть в том
                        ///< же регистре, как указано в таблице (протокол).
                        ///< - 2: При представлении числа с плавающей точкой
                        ///< (запятой) в виде строки – в качестве символа-
                        ///< разделителя целой и дробной частей использовать
                        ///< только точку, невзирая на текущую локаль на ПК
                        ///< (например, значение 12,343454 должно быть
                        ///< представлено как "12.343454").
                        ///< - 3: Порядок расположения пар ключ/значение внутри
                        ///< сообщения может быть произвольным.
    /**
     * @p Таблица описания возможностей модуля
     * @brief Конфигурация радиолокационного канала модуля
     *
     * @details Параметры для описания характеристик РЛ-канала в составе модуля
     *
     * <table>
     * <caption>Описание ключей блока данных о возможностях одной камеры
     * </caption>
     * <tr><th>Поле</th><th>Тип</th><th>Описание</th><th>Наличие поля</th></tr>
     *
     * <tr><td>Number</td><td>integer</td>
     * <td>Условный номер РЛ-канала (0 ≤ value ≤ N-1), где:<br>
     * - N: общее количество каналов<br>
     * - Максимальное значение: 255</td>
     * <td><b>Обязательное</b></td></tr>
     *
     * <tr><td>IsRotate</td><td>bool</td>
     * <td>Признак поворотного РЛ-канала:<br>
     * - 0: РЛ-канал стационарный,<br>
     * - 1: РЛ-канал может поворачиваться.</td>
     * <td><i>Опциональное</i><br>(по умолчанию: 0)</td></tr>
     *
     * <tr><td>Name</td><td>string</td>
     * <td>Условное название канала, для отображения оператору, например
     * "РЛС-Н №1"</td>
     * <td><i>Опциональное</i></td></tr>
     *
     * <tr><td>AzMin</td><td>double</td>
     * <td rowspan="2">Ширина зоны обзора по азимуту для стационарного<br>
     * канала или диапазон возможных направлений для поворотного канала.<br>
     * Значение передается в градусах относительно географического севера.<br>
     * Передача значений от 0 до 360 означает, возможность работы в любом<br>
     * азимутальном направлении.
     * </td>
     * <td rowspan="2"><b>Обязательное</b></td></tr>
     *
     * <tr><td>AzMax</td><td>double</td></tr>
     *
     * <tr><td>ElevMin</td><td>double</td>
     * <td rowspan="2">Ширина зоны обзора по углу места для стационарного<br>
     * канала или диапазон возможных углов места для поворотного канала.<br>
     * Значение передается в градусах.
     * </td>
     * <td rowspan="2"><b>Обязательное</b></td></tr>
     *
     * <tr><td>ElevMax</td><td>double</td></tr>
     *
     * <tr><td>MaxRange</td><td>double</td>
     * <td>Максимальная инструментальная дальность обнаружения<br>
     * Единицы измерения: километры (км)
     * </td>
     * <td><b>Обязательное</b></td></tr>
     * </table>
     */

    /**
     * @struct ModuleCapability
     * @brief Структура, определяющая один JSON-объект
     * @note Необходимо для вложенности
     */
    struct ModuleCapability {
        int number;
        bool isRotate = true;
        std::string name;
        double azMin = 0.0;
        double azMax = 360.0;
        double elevMin = -5.0;
        double elevMax = 90.0;
        double maxRange = 20.0;

        /**
         * @brief Базовый конструктор для возможностей РЛМН
         */
        ModuleCapability() = default;

        /**
         * @brief Конструктор для получения значений полей для каждой РЛМН
         */
        ModuleCapability(AppConfig::ModulePPU_num numberRLMN) {
            number = (int)numberRLMN;
            name = "RLS-N #" + std::to_string((int)numberRLMN);
        }

        /**
         * @brief Процедура выдачи объекта в формате JSON
         * @return Объект json
         */
        json toJson() const {
            return {
                {"Number", number},
                {"IsRotate", isRotate},
                {"Name", name},
                {"AzMin", azMin},
                {"AzMax", azMax},
                {"ElevMin", elevMin},
                {"ElevMax", elevMax},
                {"MaxRange", maxRange}
            };
        }

        /**
         * @brief Метод выдачи структуры из JSON
         * @param json& j - Входной параметр типа nlohmann::json
         * @return cap - Экземпляр ModuleCapability
         */
        static ModuleCapability fromJson(const json& j) {
            ModuleCapability cap;
            cap.number = j.at("Number").get<int>();
            cap.isRotate = j.value("IsRotate", false);
            cap.name = j.value("Name", "");
            cap.azMin = j.at("AzMin").get<double>();
            cap.azMax = j.at("AzMax").get<double>();
            cap.elevMin = j.at("ElevMin").get<double>();
            cap.elevMax = j.at("ElevMax").get<double>();
            cap.maxRange = j.at("MaxRange").get<double>();
            return cap;
        }
    };

    /**
     * @brief Конструктор из готового JSON
     * @param jsonStr Валидная JSON-строка, соответствующая спецификации
     */
    explicit TcpMessage_0x1023(const std::string& jsonStr) {
        initializeFromJson(json::parse(jsonStr));
    }

    /**
     * @brief Конструктор из JSON-объекта
     * @param jsonObj Валидный JSON-объект
     */
    explicit TcpMessage_0x1023(const json& jsonObj) {
        initializeFromJson(jsonObj);
    }

    /**
     * @brief Конструктор из списка возможностей
     * @param capabilities Вектор структур с данными
     */
    explicit TcpMessage_0x1023(const std::vector<ModuleCapability>&
                                   capabilities) {
        json jsonArr = json::array();
        for (const auto& cap : capabilities) {
            jsonArr.push_back(cap.toJson());
        }
        initializeFromJson(jsonArr);
    }

    /// @brief Деструктор
    ~TcpMessage_0x1023() {
        delete[] jsonData;
    }

    /**
     * @brief Получение данных в виде структур
     * @return Вектор объектов ModuleCapability
     * @throws std::runtime_error При ошибках валидации
     */
    std::vector<ModuleCapability> getCapabilities() const {
        std::vector<ModuleCapability> result;
        const auto jsonArr = getValidatedJson();

        for (const auto& item : jsonArr) {
            result.push_back(ModuleCapability::fromJson(item));
        }

        return result;
    }

    /**
     * @brief Полная сериализация сообщения
     * @warning Переопределяем метод сереализации для этого сообщения, т.к.
     * сейчас мы храним указатель на строку формата json
     * @return Массив Байт из json-строки
     */
    QByteArray serializeFull() const  {
        QByteArray buffer;
        const size_t jsonSize = getJsonSize();

        // sizeData
        // Фиксированная часть
        buffer.append(reinterpret_cast<const char*>(this),
                      sizeof(TcpHeader));

        // JSON-данные
        if (jsonSize > 0) {
            buffer.append(jsonData, static_cast<int>(jsonSize));
        }

        return buffer;
    }

/// @warning Отключение диагностики комплилятора для метода копирования
/// @note Мы сами перепроверяем строку при её сериализации/десериализации
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"

    /**
     * @brief Метод десериализации из пришедших в массиве байт
     * @param data - массив байт
     * @return msg - экземпляр структуры TcpMessage_0x1023
     */
    static TcpMessage_0x1023 deserialize(const QByteArray& data) {
        TcpMessage_0x1023 msg;

        // Копирование заголовка
        const size_t headerSize = sizeof(TcpMessage_0x1023);
        memcpy(&msg, data.constData(), headerSize);

        // Копирование JSON
        const size_t jsonSize = msg.getJsonSize();
        if (jsonSize > 0) {
            msg.jsonData = new char[jsonSize + 1];
            memcpy(msg.jsonData, data.constData() + headerSize, jsonSize);
            msg.jsonData[jsonSize] = '\0';
        }

        // Создаем json объект из данных и передаем его в validateJson
        json jsonObj = json::parse(msg.jsonData, msg.jsonData + jsonSize);
        validateJson(jsonObj); // Проверка после десериализации
        return msg;
    }
#pragma GCC diagnostic pop

    std::string toString() const {
        std::ostringstream oss;
        oss << TcpHeader::toString() << "\n"
            << "Module Capabilities (0x1023):\n"
            << "  JSON Data Size: " << getJsonSize() << " bytes\n";

        try {
            auto caps = getCapabilities();
            for (const auto& cap : caps) {
                oss << "\n  Capability for RLS Channel " << cap.number << ":\n"
                    << "    Name: " << cap.name << "\n"
                    << "    Is Rotatable: "
                        << (cap.isRotate ? "Yes" : "No")
                        << "\n"
                    << "    Azimuth Range: "
                        << cap.azMin << "° to " << cap.azMax << "°\n"
                    << "    Elevation Range: "
                        << cap.elevMin << "° to " << cap.elevMax << "°\n"
                    << "    Max Range: " << cap.maxRange << " km";
            }
        } catch (const std::exception& e) {
            oss << "\n  Failed to parse JSON: " << e.what();
        }
        return oss.str();
    }

private:

    /**
     * @brief initializeFromJson - метод преобразования json-объекта
     * в структуре
     * @param jsonObj - nlohmann::json
     * @private
     */
    void initializeFromJson(const json& jsonObj) {

        validateJson(jsonObj);

        const std::string jsonStr = jsonObj.dump();
        const size_t jsonSize = jsonStr.size();

        // Инициализация заголовка с правильным размером данных
        sizeData = jsonSize;  // Только размер JSON, без размера структуры

        // Выделение памяти и копирование JSON
        jsonData = new char[jsonSize + 1];
        memcpy(jsonData, jsonStr.c_str(), jsonSize);
        jsonData[jsonSize] = '\0';

        // Вычисление контрольной суммы после инициализации всех полей
        const size_t headerSize = sizeof(TcpHeader) - sizeof(checkSum);
        checkSum = calculateChecksum(reinterpret_cast<const uint8_t*>(this),
                                                    headerSize);
    }

    /**
     * @brief serializeJson - Сериализация json-объекта с учётом требований к
     * строке
     * @param j - nlohmann::json
     * @private
     * @return Возвращает преобразованную строку
     */
    static std::string serializeJson(const json& j) {
        std::ostringstream oss;
        oss.imbue(std::locale("C"));
        oss << std::fixed << std::setprecision(6);
        oss << j;
        return oss.str();
    }

    /**
     * @brief getValidatedJson - процедура получения JSON-объекта с описанной
     * валидацией
     * @return nlohmann::json (Валидный)
     */
    json getValidatedJson() const {
        const auto json = json::parse(jsonData, jsonData + getJsonSize());
        validateJson(json);
        return json;
    }

    /**
     * @brief getJsonSize - метод получения размера JSON-строки
     * @return size_t
     */
    size_t getJsonSize() const {
        return sizeData;
    }

    /**
     * @brief validateJson - головная процедура валидации JSON-а
     * @param j - объект JSON
     */
    static void validateJson(const json& j) {

        if (!j.is_array()) {
            throw std::runtime_error("Root must be JSON array");
        }

        for (const auto& item : j) {
            validateObject(item);
        }
    }

    /**
     * @brief validateObject - процедура проверки внутренних полей JSON-объекта
     * @param obj - объект JSON
     */
    static void validateObject(const json& obj) {
        // Проверка обязательных полей
        for (const auto& field : {"Number", "AzMin", "AzMax",
                                  "ElevMin", "ElevMax", "MaxRange"}) {
            if (!obj.contains(field)) {
                throw std::runtime_error("Missing required field: " +
                                         std::string(field));
            }
        }

        // Валидация значений
        const int number = obj["Number"].get<int>();
        if (number < 0 || number > 255) {
            throw std::runtime_error("Invalid Number value: " +
                                     std::to_string(number));
        }

        const double azMin = obj["AzMin"].get<double>();
        const double azMax = obj["AzMax"].get<double>();
        if (azMin < 0.0 || azMax > 360.0 || azMin > azMax) {
            throw std::runtime_error("Invalid azimuth range: [" +
                                     std::to_string(azMin) + ", " +
                                     std::to_string(azMax) + "]");
        }

        const double elevMin = obj["ElevMin"].get<double>();
        const double elevMax = obj["ElevMax"].get<double>();
        if (elevMin < -90.0 || elevMax > 90.0 || elevMin > elevMax) {
            throw std::runtime_error("Invalid elevation range: [" +
                                     std::to_string(elevMin) + ", " +
                                     std::to_string(elevMax) + "]");
        }

        if (obj["MaxRange"].get<double>() <= 0.0) {
            throw std::runtime_error("MaxRange must be positive");
        }
    }

    TcpMessage_0x1023() : jsonData(nullptr) {} // Для десериализации
};
#pragma pack(pop)

static_assert(sizeof(TcpMessage_0x1023) == 16 + sizeof(char*),
              "Invalid TcpMessage_0x1023 size");

#pragma pack(push, 1)
/**
 * @struct TcpMessage_0x1024
 * @brief Сообщение о запрещенных секторах (0x1024)
 */
struct TcpMessage_0x1024 : TcpMessage<TcpMessage_0x1024,
                                      PBU_MessageNumbers::FORBIDDEN_SECTORS> {
    /**
     * @struct Sector
     * @brief Описание одного запрещенного сектора
     */
    struct Sector {
        float limAzBegin;       ///< @brief Азимут начала сектора [град]
                                ///< @details Передается в градусах,
                                ///< относительно географического севера
                                ///< относительно точки стояния РЛС-Н
                                ///< @note 0 - направление на север;
                                ///< @warning Положительное направление
                                ///< отсчитывается по часовой стрелке.

        float limAzEnd;         ///< @brief Азимут конца сектора [град]
                                ///< @details передается в градусах,
                                ///< относительно географического севера
                                ///< относительно точки стояния РЛС-Н
                                ///< @warning направление от начального значения
                                ///< отсчитывается вдоль положительного
                                ///< направления координатной оси (по часовой
                                ///< стрелке).

        float limElevBegin;     ///< @brief Угол места начала сектора [град]
                                ///< @details передается в градусах,
                                ///< относительно точки стояния РЛС-Н
                                ///< @note 0 – направление в горизонт,
                                ///< 90 – направление в зенит


        float limElevEnd;       ///< @brief Угол места конца сектора [град]
                                ///< @details передается в градусах,
                                ///< относительно точки стояния РЛС-Н
                                ///< @warning направление от начального значения
                                ///< отсчитывается вдоль положительного
                                ///< направления координатной оси (от горизонта
                                ///< к зениту).

        /// @brief Конструктор по-умолчанию
        Sector() = default;

        /// @brief Конструктор с входными параметрами
        Sector(float azBegin, float azEnd, float elevBegin, float elevEnd) {
            if (azBegin > azEnd || elevBegin > elevEnd) {
                throw std::invalid_argument("Invalid sector parameters");
            }
            limAzBegin = azBegin;
            limAzEnd = azEnd;
            limElevBegin = elevBegin;
            limElevEnd = elevEnd;
        }
    };

    std::vector<Sector> sectors; ///< @brief Список запрещенных секторов

    /**
     * @brief Конструктор по умолчанию
     */
    TcpMessage_0x1024() = default;

    /**
     * @brief Конструктор с инициализацией секторов
     */
    explicit TcpMessage_0x1024(const std::vector<Sector>& sectorsList) {
        for (const auto& sector : sectorsList) {
            addSector(sector);
        }
    }

    /**
     * @brief Добавление сектора с проверкой параметров
     */
    void addSector(const Sector& sector) {
        if (sector.limAzBegin > sector.limAzEnd ||
            sector.limElevBegin > sector.limElevEnd) {
            throw std::invalid_argument("Invalid sector parameters");
        }
        sectors.push_back(sector);
        updateDataSize();
    }

    /**
     * @brief Сериализация сообщения
     */
    QByteArray serializeFull() const {
        QByteArray data;

        // Сериализация заголовка
        data.append(this->serialize());

        // Сериализация секторов
        for (const auto& sector : sectors) {
            data.append(reinterpret_cast<const char*>(&sector), sizeof(Sector));
        }

        return data;
    }

    /**
     * @brief Десериализация сообщения
     */
    static TcpMessage_0x1024 deserialize(const QByteArray& data) {
        TcpMessage_0x1024 message;

        // Десериализация заголовка
        TcpHeader header = TcpHeader::deserialize(data);
        if (!header.isValid()) {
            throw std::runtime_error("Invalid header");
        }

        // Проверка размера данных
        const size_t sectorsCount = (data.size() - sizeof(TcpHeader)) /
                                    sizeof(Sector);
        if ((data.size() - sizeof(TcpHeader)) % sizeof(Sector) != 0) {
            throw std::runtime_error("Invalid data size");
        }

        // Десериализация секторов
        const char* sectorsData = data.constData() + sizeof(TcpHeader);
        for (size_t i = 0; i < sectorsCount; ++i) {
            Sector sector;
            memcpy(&sector, sectorsData + i * sizeof(Sector), sizeof(Sector));
            message.sectors.push_back(sector);
        }

        return message;
    }

    /**
     * @brief Получение сектора по индексу
     */
    const Sector& getSector(size_t index) const {
        if (index >= sectors.size()) {
            throw std::out_of_range("Sector index out of range");
        }
        return sectors[index];
    }

    std::string toString() const {
        std::ostringstream oss;
        oss << TcpHeader::toString() << "\n"
            << "Forbidden Sectors (0x1024):\n"
            << "  Sectors Count: " << sectors.size() << "\n";

        for (size_t i = 0; i < sectors.size(); ++i) {
            const auto& sector = sectors[i];
            oss << "\n  Sector #" << i << ":\n"
                << "    Azimuth Range: " << sector.limAzBegin << "° to "
                << sector.limAzEnd << "°\n"
                << "    Elevation Range: " << sector.limElevBegin << "° to "
                << sector.limElevEnd << "°";
        }
        return oss.str();
    }

private:
    /**
     * @brief Обновление размера данных в заголовке
     */
    void updateDataSize() {
        sizeData = sectors.size() * sizeof(Sector);
    }
};
#pragma pack(pop)

static_assert(sizeof(TcpMessage_0x1024::Sector) == 16,
              "Invalid Sector size");
static_assert(sizeof(TcpMessage_0x1024) == sizeof(TcpHeader) +
                                sizeof(std::vector<TcpMessage_0x1024::Sector>),
              "Invalid TcpMessage_0x1024 size");

// #pragma pack(push, 1)
// /**
//  * @struct TcpMessage_0x09
//  * @brief Отметка трассы
//  * @warning Пока НЕ трогаем
//  */
// #pragma pack(pop)

// -----------------------------------------
// Сообщения от ПБУ на ВУ [ОБРАБОТКА ИНФОРМАЦИИ]
// -----------------------------------------
#pragma pack(push, 1)
/**
 * @struct TcpMessage_0x02
 * @brief Подтверждение регистрации модуля в системе
 */
struct TcpMessage_0x02 : TcpMessage<TcpMessage_0x02,
                                    PBU_MessageNumbers::RES_REGISTRATION> {
    uint8_t idxModule;      ///< @brief Номер модуля в комплексе
                            ///< @details В данное поле передается условный
                            ///< номер модуля в комплексе. При последующем
                            ///< обмене в поле idxModule (п. 3.1) общего
                            ///< заголовка сообщений от ПО модуля допускается
                            ///< только указанное в данном сообщении значение.

    uint8_t errorConnect;   ///< @brief Статус ошибки регистрации
                            ///< @details Передается код ошибки регистрации
                            ///< модуля в системе.
                            ///< @warning Подключение считается установленным,
                            ///< **только** при значении «0» в данном поле.
                            ///< @note Коды ошибок интерпретируются битовой
                            ///< маской:
                            ///< - Бит 1: Превышен лимит на кол-во модулей
                            ///< данного типа;
                            ///< - Бит 2: Попытка повторного подключения
                            ///< идентичного устройства;
                            ///< - Бит 3: Не поддерживаемый тип модуля;
                            ///< - Бит 4: Не поддерживаемая версия протокола;
                            ///< - Бит 5: Не поддерживаемый номер модуля в
                            ///< сообщении 0х1 «Запрос регистрации модуля в
                            ///< системе»;
                            ///< - Бит 6-8: Неизвестная ошибка (резерв).
                            ///< @warning При наличии ошибки в данном поле,
                            ///< TCP-соединение разрывается. Повторная
                            ///< регистрация со стороны клиента допускается не
                            ///< ранее чем через 10с.

    uint16_t reserve;       ///< @brief Резервное поле (не используется)

    std::string toString() const {
        std::ostringstream oss;
        oss << TcpHeader::toString() << "\n"
            << "Registration Response (0x02):\n"
            << "  Module Index: " << static_cast<int>(idxModule) << "\n"
            << "  Error Code: 0x" << std::hex << static_cast<int>(errorConnect)
            << std::dec << "\n";

        // Расшифровка ошибки
        if (errorConnect == 0) {
            oss << "  Status: Registration successful";
        } else {
            oss << "  Error Details:";
            if (errorConnect & 0x01) oss << "\n    - Module limit exceeded";
            if (errorConnect & 0x02) oss << "\n    - Duplicate device";
            if (errorConnect & 0x04) oss << "\n    - Unsupported module type";
            if (errorConnect & 0x08) oss << "\n    - Protocol version mismatch";
            if (errorConnect & 0x10) oss << "\n    - Invalid module number";
            if (errorConnect & 0xE0) oss << "\n    - Unknown error";
        }
        return oss.str();
    }
};
#pragma pack(pop)

static_assert(sizeof(TcpMessage_0x02) == 16 + 4,
              "Invalid size of TcpMessage_0x02");

#pragma pack(push, 1)
/**
 * @struct TcpMessage_0x04
 * @brief Отметка времени
 * @details Данный тип сообщения отправляется только при поступлении
 * соответствующего запроса от ПО модуля «Запрос времени»
 * @warning Если в течение 2 секунд от ПО модуля не поступают запросы времени,
 * то связь с ПО модуля считается разорванной. В этом случае TCP-соединение
 * принудительно разрывается и ожидается новое подключение с последующей
 * регистрацией модуля в системе.
 */
struct TcpMessage_0x04 : TcpMessage<TcpMessage_0x04,
                                    PBU_MessageNumbers::TIMESTAMP> {
    uint32_t timeRequest1;      ///< Метка времени запроса младшие байты
    uint32_t timeRequest2;      ///< Метка времени запроса старшие байты
    uint32_t timeAnswer1;       ///< Метка времени ответа младшие байты
    uint32_t timeAnswer2;       ///< Метка времени ответа старшие байты

    std::string toString() const {
        std::ostringstream oss;
        uint64_t requestTime = (static_cast<uint64_t>(timeRequest2) << 32) | timeRequest1;
        uint64_t answerTime = (static_cast<uint64_t>(timeAnswer2) << 32) | timeAnswer1;
        oss << TcpHeader::toString() << "\n"
            << "Timestamp (0x04):\n"
            << "  Request Time: " << requestTime << " ms ("
            << TimeProvider::instance().msToDateTime(requestTime) << ")\n"
            << "  Answer Time: " << answerTime << " ms ("
            << TimeProvider::instance().msToDateTime(answerTime) << ")\n"
            << "  Latency: " << TimeProvider::instance()
                            .msToDateTime(answerTime - requestTime) << " ms";
        return oss.str();
    }
};
#pragma pack(pop)

static_assert(sizeof(TcpMessage_0x04) == 16 + 16,
              "Invalid size of TcpMessage_0x04");

// #pragma pack(push, 1)
// /**
//  * @struct TcpMessage_0x06
//  * @brief Геопозиция комплекса
//  * @warning Пока НЕ трогаем
//  */
// #pragma pack(pop)

// #pragma pack(push, 1)
// /**
//  * @struct TcpMessage_0x08
//  * @brief Ориентация СК комплекса
//  * @warning Пока НЕ трогаем
//  */
// #pragma pack(pop)

#pragma pack(push, 1)
/**
 * @struct TcpMessage_0x07
 * @brief Сообщение отметки трассы от ПБУ (0x07)
 */
struct TcpMessage_0x07 : TcpMessage<TcpMessage_0x07,
                                    PBU_MessageNumbers::MARK_TRAICE_PBU> {
    uint32_t time1;     ///< @brief Младшие 32 бита временной метки
    uint32_t time2;     ///< @brief Старшие 32 бита временной метки
    uint16_t cellVoi;   ///< @brief Порядковый номер цели модуля (16 бит)
    uint16_t cellKOIR;  ///< @brief Порядковый номер цели боевого/оптического
                        ///< модуля (16 бит)

    uint8_t ktPrizn;    ///< @brief Признаки обнаружения
                        ///< @details В данное поле передается набор признаков
                        ///< обнаружение цели по разным каналам в текущем обзоре
                        ///< в соответствии с битовой маской:
                        ///< - 1 бит: Обнаружение ПРЛ
                        ///< - 2 бит: Обнаружение радиотехнической разведкой
                        ///< - 3 бит: Обнаружение боевым/оптическим модулем
                        ///< - 4 бит: Обнаружение АЗН
                        ///< - 5 бит: Телеметрия от НСУ
                        ///< - 6 бит: Обнаружение ВРЛ
                        ///< - 7-8 биты: Свой-чужой:
                        ///<   - «00»: Неопознанная цель
                        ///<   - «01»: Своя
                        ///<   - «10»: Чужая
                        ///< @warning Если в битах с 1 по 8 стоят нули, значит,
                        ///< в данном обзоре не было обнаружения ни по одному
                        ///< каналу. В этом случае в полях координат передаются
                        ///< экстраполированные координаты цели.

    uint8_t trPrizn;    ///< @brief Признаки сопровождения
                        ///< @details Передается набор признаков сопровождения
                        ///< цели по данным разных источников данных в
                        ///< соответствии с битовой маской:
                        ///< - 1 бит: По данным ПРЛ
                        ///< - 2 бит: По данным радиотехнической разведки
                        ///< - 3 бит: По данным боевого/оптического модуля
                        ///< - 4 бит: По данным АЗН
                        ///< - 5 бит: По данным телеметрии от НСУ
                        ///< - 6 бит: По данным ВРЛ
                        ///< - 7 бит: По данным НРЗ
                        ///< - 8 бит: резерв

    uint8_t typeCeil;   ///< @brief Тип цели
                        ///< @details В данное поле передается определенный в ВУ
                        ///< тип цели.
                        ///< Значения:
                        ///< - «1»: птица;
                        ///< - «2»: стая птиц;
                        ///< - «3»: БпЛА типа самолет;
                        ///< - «4»: БпЛА типа квадрокоптер;
                        ///< - «5»: ракета;
                        ///< - «6»: вертолет;
                        ///< - «7»: самолет;
                        ///< - «8»: ложная цель;
                        ///< - «10»: искусственный объект;
                        ///< - «11»: БпЛА;
                        ///< - «13»: морское/речное судно;
                        ///< - «14»: наземный объект (автомобиль);
                        ///< - «255»:  тип не определен.

    uint8_t typeInfo: 2;    ///< @brief Статус информации о трассе
                            ///< @details Передается код статуса информации:
                            ///< - «0»: Новая трасса;
                            ///< - «1»: Обновление информации по ранее
                            ///< сопровождавшейся трассе;
                            ///< - «2»: Сброс трассы (срыв сопровождения).

    uint8_t isTest: 1;      ///< @brief Признак тестовой цели
                            ///< @details Устанавливается в «1» в случае, если
                            ///< передаваемая информация соответствует тестовой
                            ///< (имитационной) цели, иначе передается «0»

    uint8_t isKursMan: 1;   ///< @brief Признак маневра по курсу
                            ///< @details Устанавливается в «1» в случае, если
                            ///< для сопровождаемая цель выполняет полет с
                            ///< маневрированием, иначе передается «0»

    uint8_t isHMan: 2;      ///< @brief Признак изменения высоты
                            ///< @details Описание значений:
                            ///< - «0»: высота не изменяется.
                            ///< - «1»: увеличение высоты.
                            ///< - «2»: уменьшение высоты.
                            ///< - «3»: резерв.

    uint8_t isH: 1;         ///< @brief Достоверность высоты
                            ///< @details Булево значение

    uint8_t isVr: 1;        ///< @brief Признак достоверности радиальной
                            ///< скорости цели
                            ///< @details Описание значений:
                            ///< - «0»: значение радиальной скорости цели не
                            ///< определено;
                            ///< - «1»: значение радиальной скорости цели
                            ///< достоверно.

    float ceilX;            ///< @brief Координата цели по оси X
    float ceilY;            ///< @brief Координата цели по оси Y
    float ceilZ;            ///< @brief Координата цели по оси Z
    float latitude;         ///< @brief Широта
    float longitude;        ///< @brief Долгота
    float height;           ///< @brief Высота

    float sizeAz;           ///< @brief Размер строба по азимуту (в градусах)
    float sizeD;            ///< @brief Размер строба по дальности (в метрах)

    /**
     * @brief Вектор скорости цели
     * @details Проекции вектора скорости перемещения цели на оси:
     * - X (широта)
     * - Y (долгота)
     * - Z (высота)
     *
     * ```
     *        ^ Z (высота)
     *        |
     *        |
     *        +-----> Y (долгота)
     *       /
     *      /
     *     X (широта)
     * ```
     *
     * Единицы измерения: м/с
     */
    float velX;             ///< @brief Скорость цели (X - широта)
    float velY;             ///< @brief Скорость цели (Y - долгота)
    float velZ;             ///< @brief Скорость цели (Z - высота)

    float velR;             ///< @brief Радиальная скорость цели
                            ///< @details Передается оценка средней радиальной
                            ///< скорости цели относительно РЛС модуля в м/с

    uint32_t ceilPel;       ///< @brief Номер трассы радиотехнической разведки
                            ///< @details Передается порядковый номер трассы в
                            ///< нумерации ПО радиотехнической разведки в том
                            ///< случае, если в признаках сопровождения стоит
                            ///< флаг сопровождения по данным радиотехнической
                            ///< разведки.
                            ///< @warning Если флаг сопровождения по данным
                            ///< радиотехнической разведки не стоит, данное поле
                            ///< должно игнорироваться.

    uint32_t ceilAddr: 24;  ///< @brief Адрес цели от АЗН
                            ///< @details Уникальный номер воздушного судна в
                            ///< том случае, если в признаках сопровождения
                            ///< стоит флаг сопровождения по данным АЗН.
                            ///< @warning Если флаг сопровождения по данным АЗН
                            ///< не стоит, данное поле должно игнорироваться.

    uint32_t ceilCat: 8;    ///< @brief Категория цели от АЗН
                            ///< @details Тип воздушного судна в том случае,
                            ///< если в признаках сопровождения стоит флаг
                            ///< сопровождения по данным АЗН.
                            ///< @warning Если флаг сопровождения по данным АЗН
                            ///< не стоит, данное поле должно игнорироваться.

    uint32_t reserve;       ///< @brief Резервное поле


    std::string toString() const {
        std::ostringstream oss;
        uint64_t time = (static_cast<uint64_t>(time2) << 32) | time1;
        oss << TcpHeader::toString() << "\n"
            << "PBU Track Mark (0x07):\n"
            << "  Time: " << TimeProvider::instance().msToDateTime(time) << "\n"
            << "  Module Track ID: " << cellVoi << "\n"
            << "  KOIR Track ID: " << cellKOIR << "\n"
            << "  Detection Flags: 0x" << std::hex << static_cast<int>(ktPrizn)
                << std::dec << "\n"
            << "  Tracking Flags: 0x" << std::hex << static_cast<int>(trPrizn)
                << std::dec << "\n"
            << "  Target Type: " << static_cast<int>(typeCeil) << "\n"
            << "  Info Status: " << static_cast<int>(typeInfo) << "\n"
            << "  Is Test: " << (isTest ? "Yes" : "No") << "\n"
            << "  Is Course Maneuver: " << (isKursMan ? "Yes" : "No") << "\n"
            << "  Height Change: " << static_cast<int>(isHMan) << "\n"
            << "  Height Valid: " << (isH ? "Yes" : "No") << "\n"
            << "  Velocity Valid: " << (isVr ? "Yes" : "No") << "\n"
            << "  Coordinates (X,Y,Z): " << ceilX << ", " << ceilY << ", "
                << ceilZ << "\n"
            << "  Geoposition (Lat,Lon,Alt): " << latitude << ", "
                << longitude << ", " << height << "\n"
            << "  Azimuth Size: " << sizeAz << "°\n"
            << "  Range Size: " << sizeD << " m\n"
            << "  Velocity (X,Y,Z): " << velX << ", " << velY << ", "
                << velZ << " m/s\n"
            << "  Radial Velocity: " << velR << " m/s\n"
            << "  RT Track Number: " << ceilPel << "\n"
            << "  ADS-B Address: " << std::hex << (ceilAddr & 0xFFFFFF)
                << std::dec << "\n"
            << "  ADS-B Category: " << static_cast<int>(ceilCat) << "\n"
            << "  Reserved: " << reserve;
        return oss.str();
    }
};
#pragma pack(pop)

static_assert(sizeof(TcpMessage_0x07) == 16 + 19*4,
              "Invalid size [TcpMessage_0x07]");

#pragma pack(push, 1)
/**
 * @struct TcpMessage_0x641
 * @brief Команда «Целеуказание»
 */
struct TcpMessage_0x641 : TcpMessage<TcpMessage_0x641,
                                     PBU_MessageNumbers::TARGET_DESIGNATION> {
    /// @name Метка времени цели
    /// @details В данное поле записывается UTC время ПО СОИ на момент
    /// определения первичных координат цели.
    /// @note Передается в виде 64-битного значения кол-ва миллисекунд,
    /// прошедших с 00:00:00.000 1970 01 01 (Unix Epoch)
    /// @{
    uint32_t time1;             ///< @brief Младшие 32 бита временной метки

    uint32_t time2;             ///< @brief Старшие 32 бита временной метки
    /// @}

    uint16_t idxCeilVOI;        ///< @brief Порядковый номер трассы СОИ
                                ///< @details Cквозной порядковый номер трассы
                                ///< по нумерации ПО СОИ

    uint16_t numDrive : 2;      ///< @brief Номер РЛ-канала
                                ///< @details Передается номер РЛ-канала для
                                ///< которого необходимо применить команду
                                ///< целеуказания

    uint16_t typeCeil : 3;      ///< @brief Тип целеуказания
                                ///< @details Передается тип (характер) цели для
                                ///< которой выдается целеуказание.
                                ///< @note Поле может содержать только следующие
                                ///< значения:
                                ///< - "0": первичное целеуказание для уточнения
                                ///< параметров цели;
                                ///< - "1": выдача целеуказания на
                                ///< обстреливаемую цель;
                                ///< - "2": выдача целеуказания на средство
                                ///< поражения (БпЛА-перехватчик);
                                ///< - "3": отмена целеуказания (прекратить
                                ///< поиск и сопровождение).


    uint16_t isModel  : 1;      ///< @brief Признак моделирования
                                ///< @details В данном поле передается признак
                                ///< того, что целеуказание соответствует
                                ///< моделируемому объекту (bool)

    uint16_t reserve : 10;      ///< @brief Резервное поле (не используется)

    float ceilAz;               ///< @brief Азимут цели
                                ///< @details Передается в градусах относительно
                                ///< географического севера относительно точки
                                ///< стояния РЛС-Н

    float ceilElev;             ///< @brief Угол места цели
                                ///< @details В данном поле передается угол
                                ///< места цели в градусах относительно точки
                                ///< стояния РЛС-Н

    float ceilRange;            ///< @brief Наклонная радиальная дальность цели
                                ///< @details Передается в метрах относительно
                                ///< точки стояния РЛС-Н

    float ceilVr;               ///< @brief Радиальная скорость цели
                                ///< @details передается в м/с, положительная
                                ///< скорость означает движение цели в
                                ///< направлении от РЛС, отрицательная – на РЛС.

    float ceilVAz;              ///< @brief Скорость изменения азимута объекта
                                ///< (град/с)

    float ceilVElev;            ///< @brief Скорость изменения угла места
                                ///< объекта (град/с)

    float sigAz;                ///< @brief Ошибка координат объекта по азимуту
                                ///< (град)

    float sigElev;              ///< @brief Ошибка координат объекта по углу
                                ///< места (град)

    float sigRange;             ///< @brief Ошибка координат объекта по
                                ///< дальности (метры)

    float sigVAz;               ///< @brief Ошибка скорости изменения азимута
                                ///< объекта (град/с)

    float sigVElev;             ///< @brief Ошибка скорости изменения угла места
                                ///< объекта (град/с)

    float sigVr;                ///< @brief Ошибка скорости изменения дальности
                                ///< объекта (м/с)

    uint32_t numBPLA;           ///< @brief Серийный номер перехватчика
                                ///< @details В данном поле передается серийный
                                ///< номер перехватчика
                                ///< @warning Если в поле "Тип целеуказания"
                                ///< указан признак перехватчика (при любом
                                ///< другом признаке поле Серийный номер
                                ///< перехватчика игнорируется).
    /**
     * @note В ответ на данную команду ПО модуля должно отправить квитанцию
     * (сообщение 0х23) либо расширенную квитанцию (сообщение 0х27) о выполнении
     * команды в ПО СОИ:
     * - "0": команда выполнена без ошибок;
     * - "1": команда в процессе выполнения;
     * - "2": некорректные параметры — передается в случае некорректности хотя
     * бы одного из параметров;
     * - "3": выполнение команды прервано новой командой управления либо
     * отменено оператором;
     * - "4": команда не выполнена по причине неисправности оборудования;
     * - "5": команда не выполнена из-за внутренней ошибки;
     * - "7": команда отклонена, так как РАФ выключен;
     * - "8": команда не выполнена по причине неготовности оборудования;
     * - "9": команда не выполнена, так как модуль находится в местном режиме;
     * - "127": неизвестная ошибка;
     * - "256": цель не обнаружена;
     * - "257": выход за пределы возможной зоны обзора — передается в случае
     * превышения граничных значений положения ОПУ в азимутальной и угломестной
     * плоскостях.
     */

    std::string toString() const {
        std::ostringstream oss;
        uint64_t time = (static_cast<uint64_t>(time2) << 32) | time1;
        oss << TcpHeader::toString() << "\n"
            << "Target Designation (0x641):\n"
            << "  Detection Time: " << TimeProvider::instance()
                                                    .msToDateTime(time) << "\n"
            << "  VOI Track Index: " << idxCeilVOI << "\n"
            << "  RLS Channel: " << static_cast<int>(numDrive) << "\n"
            << "  Target Type: ";

        switch (typeCeil) {
        case 0: oss << "Primary (target refinement)"; break;
        case 1: oss << "Attack target"; break;
        case 2: oss << "Interceptor (BPLA)"; break;
        case 3: oss << "Cancel designation"; break;
        default: oss << "Unknown"; break;
        }

        oss << "\n  Is Simulation: " << (isModel ? "Yes" : "No") << "\n"
            << "  Target Coordinates:\n"
            << "    Azimuth: " << ceilAz << "°\n"
            << "    Elevation: " << ceilElev << "°\n"
            << "    Range: " << ceilRange << " m\n"
            << "    Radial Velocity: " << ceilVr << " m/s\n"
            << "  Error Estimates:\n"
            << "    Azimuth: " << sigAz << "°\n"
            << "    Elevation: " << sigElev << "°\n"
            << "    Range: " << sigRange << " m\n"
            << "    Azimuth Velocity: " << sigVAz << "°/s\n"
            << "    Elevation Velocity: " << sigVElev << "°/s\n"
            << "    Radial Velocity: " << sigVr << " m/s";

        if (typeCeil == 2) {
            oss << "\n  BPLA Serial Number: " << numBPLA;
        }

        return oss.str();
    }
};
#pragma pack(pop)

static_assert(sizeof(TcpMessage_0x641) == 16 + 64,
              "Invalid size [TcpMessage_0x641]");

// -----------------------------------------
// Сообщения от ВУ на ПБУ [КОНТРОЛЯ И УПРАВЛЕНИЯ]
// -----------------------------------------

#pragma pack(push, 1)
/**
 * @struct TcpMessage_0x20
 * @brief Схема модуля (0x20)
 * @details Сообщение содержит описание структуры модуля в формате JSON
 */
struct TcpMessage_0x20 : TcpMessage<TcpMessage_0x20,
                                    PBU_MessageNumbers::MODULE_SCHEME> {
    char* jsonData;     ///< Указатель на сырые JSON-данные

    // Forward declarations
    struct ParamTemplate;
    struct ControlledParam;
    struct ConfigParam;
    struct Element;

    /**
     * @struct ParamTemplate
     * @brief Шаблон возможных значений параметра
     */
    struct ParamTemplate {
        int Value = 0;          ///< Значение параметра
        std::string Text;       ///< Описание значения

        json toJson() const {
            return {
                {"Value", Value},
                {"Text", Text}
            };
        }

        static ParamTemplate fromJson(const json& j) {
            ParamTemplate tpl;
            tpl.Value = j.at("Value").get<int>();
            tpl.Text = j.at("Text").get<std::string>();
            return tpl;
        }
    };

    /**
     * @struct ControlledParam
     * @brief Контролируемый параметр элемента
     */
    struct ControlledParam {
        int IDParam = 0;                        ///< Идентификатор КП
        std::string Name;                       ///< Название КП
        int Type = 0;                           ///< Тип КП (0-float, 1-text,
                                                ///<  2-int32, 3-bool, 4-group)
        std::vector<ControlledParam> Params;    ///< Параметры группы (для
                                                ///< Type=4)
        std::string Description;                ///< Описание КП
        std::string Format;                     ///< Формат вывода
        std::vector<ParamTemplate> Template;    ///< Шаблон возможных значений

        json toJson() const {
            json j;
            j["IDParam"] = IDParam;
            j["Name"] = Name;
            j["Type"] = Type;

            if (!Description.empty()) j["Description"] = Description;
            if (!Format.empty()) j["Format"] = Format;

            if (!Params.empty()) {
                json paramsArr = json::array();
                for (const auto& param : Params) {
                    paramsArr.push_back(param.toJson());
                }
                j["Params"] = paramsArr;
            }

            if (!Template.empty()) {
                json templateArr = json::array();
                for (const auto& tpl : Template) {
                    templateArr.push_back(tpl.toJson());
                }
                j["Template"] = templateArr;
            }

            return j;
        }

        static ControlledParam fromJson(const json& j) {
            ControlledParam param;
            param.IDParam = j.at("IDParam").get<int>();
            param.Name = j.at("Name").get<std::string>();
            param.Type = j.at("Type").get<int>();

            param.Description = j.value("Description", "");
            param.Format = j.value("Format", "");

            if (j.contains("Params")) {
                for (const auto& item : j["Params"]) {
                    param.Params.push_back(ControlledParam::fromJson(item));
                }
            }

            if (j.contains("Template")) {
                for (const auto& item : j["Template"]) {
                    param.Template.push_back(ParamTemplate::fromJson(item));
                }
            }

            return param;
        }
    };

    /**
     * @struct ConfigParam
     * @brief Настраиваемый параметр элемента
     */
    struct ConfigParam {
        int IDConfigParam = 0;                  ///< Идентификатор НП
        std::string NameConfig;                 ///< Название НП
        std::string Units;                      ///< Единицы измерения
        int TypeConfig = 0;                     ///< Тип НП (0-float, 1-text,
                                                ///< 2-int32, 3-bool, 4-group)
        bool IsSave = false;                    ///< Возможность сохранения
        float RangeMin = 0.0f;                  ///< Минимальное значение
        float RangeMax = 0.0f;                  ///< Максимальное значение
        std::vector<std::string> Values;        ///< Возможные значения
        std::string Description;                ///< Описание НП
        float Default = 0.0f;                   ///< Значение по умолчанию
        std::vector<ConfigParam> ConfigParams;  ///< Параметры группы (для
                                                ///< TypeConfig=4)
        int UserLevel = 0;                      ///< Уровень доступа

        json toJson() const {
            json j;
            j["IDConfigParam"] = IDConfigParam;
            j["NameConfig"] = NameConfig;
            j["TypeConfig"] = TypeConfig;
            j["IsSave"] = IsSave;

            if (!Units.empty()) j["Units"] = Units;
            if (!Description.empty()) j["Description"] = Description;

            if (TypeConfig != 4) { // Для негрупповых параметров
                if (!Values.empty()) {
                    j["Values"] = Values;
                } else if (RangeMin != 0.0f || RangeMax != 0.0f) {
                    j["RangeMin"] = RangeMin;
                    j["RangeMax"] = RangeMax;
                }

                if (Default != 0.0f) j["Default"] = Default;
            }

            if (UserLevel != 0) j["UserLevel"] = UserLevel;

            if (!ConfigParams.empty()) {
                json configArr = json::array();
                for (const auto& cfg : ConfigParams) {
                    configArr.push_back(cfg.toJson());
                }
                j["ConfigParam"] = configArr;
            }

            return j;
        }

        static ConfigParam fromJson(const json& j) {
            ConfigParam param;
            param.IDConfigParam = j.at("IDConfigParam").get<int>();
            param.NameConfig = j.at("NameConfig").get<std::string>();
            param.TypeConfig = j.at("TypeConfig").get<int>();
            param.IsSave = j.at("IsSave").get<bool>();

            param.Units = j.value("Units", "");
            param.Description = j.value("Description", "");
            param.Default = j.value("Default", 0.0f);
            param.UserLevel = j.value("UserLevel", 0);

            if (j.contains("Values")) {
                for (const auto& item : j["Values"]) {
                    param.Values.push_back(item.get<std::string>());
                }
            }

            if (j.contains("RangeMin")) param.RangeMin = j["RangeMin"]
                                     .get<float>();
            if (j.contains("RangeMax")) param.RangeMax = j["RangeMax"]
                                     .get<float>();

            if (j.contains("ConfigParam")) {
                for (const auto& item : j["ConfigParam"]) {
                    param.ConfigParams.push_back(ConfigParam::fromJson(item));
                }
            }

            return param;
        }
    };

    /**
     * @struct Element
     * @brief Описание элемента схемы
     */
    struct Element {
        int IDElem = -1;        ///< Идентификатор элемента (от 0 до 65535,
                                ///< -1 если нет)
        std::string Name;       ///< Полное название элемента
        std::string ShortName;  ///< Короткое название или аббревиатура
        std::vector<Element> Elements;  ///< Вложенные элементы
        std::vector<ControlledParam> Params;  ///< Контролируемые параметры
        std::vector<ConfigParam> ConfigParams; ///< Настраиваемые параметры

        json toJson() const {
            json j;
            if (IDElem != -1) j["IDElem"] = IDElem;
            j["Name"] = Name;
            if (!ShortName.empty()) j["ShortName"] = ShortName;

            if (!Elements.empty()) {
                json elementsArr = json::array();
                for (const auto& elem : Elements) {
                    elementsArr.push_back(elem.toJson());
                }
                j["Elements"] = elementsArr;
            }

            if (!Params.empty()) {
                json paramsArr = json::array();
                for (const auto& param : Params) {
                    paramsArr.push_back(param.toJson());
                }
                j["Params"] = paramsArr;
            }

            if (!ConfigParams.empty()) {
                json configArr = json::array();
                for (const auto& config : ConfigParams) {
                    configArr.push_back(config.toJson());
                }
                j["ConfigParam"] = configArr;
            }

            return j;
        }

        static Element fromJson(const json& j) {
            Element elem;
            elem.IDElem = j.value("IDElem", -1);
            elem.Name = j.at("Name").get<std::string>();
            elem.ShortName = j.value("ShortName", "");

            if (j.contains("Elements")) {
                for (const auto& item : j["Elements"]) {
                    elem.Elements.push_back(Element::fromJson(item));
                }
            }

            if (j.contains("Params")) {
                for (const auto& item : j["Params"]) {
                    elem.Params.push_back(ControlledParam::fromJson(item));
                }
            }

            if (j.contains("ConfigParam")) {
                for (const auto& item : j["ConfigParam"]) {
                    elem.ConfigParams.push_back(ConfigParam::fromJson(item));
                }
            }

            return elem;
        }
    };

    /**
     * @brief Конструктор из готового JSON
     * @param jsonStr Валидная JSON-строка
     */
    explicit TcpMessage_0x20(const std::string& jsonStr) {
        initializeFromJson(json::parse(jsonStr));
    }

    /**
     * @brief Конструктор из JSON-объекта
     * @param jsonObj Валидный JSON-объект
     */
    explicit TcpMessage_0x20(const json& jsonObj) {
        initializeFromJson(jsonObj);
    }

    /**
     * @brief Конструктор из списка элементов
     * @param elements Вектор структур с данными
     */
    explicit TcpMessage_0x20(const std::vector<Element>& elements) {
        json jsonArr = json::array();
        for (const auto& elem : elements) {
            jsonArr.push_back(elem.toJson());
        }
        initializeFromJson(jsonArr);
    }

    /// @brief Деструктор
    ~TcpMessage_0x20() {
        delete[] jsonData;
    }

    /**
     * @brief Получение данных в виде структур
     * @return Вектор элементов схемы
     */
    std::vector<Element> getElements() const {
        std::vector<Element> result;
        const auto jsonArr = json::parse(jsonData, jsonData + getJsonSize());

        for (const auto& item : jsonArr) {
            result.push_back(Element::fromJson(item));
        }

        return result;
    }

    /**
     * @brief Полная сериализация сообщения
     * @return Массив байт из json-строки
     */
    QByteArray serializeFull() const {
        QByteArray buffer;
        const size_t jsonSize = getJsonSize();

        // Фиксированная часть
        buffer.append(reinterpret_cast<const char*>(this),
                      sizeof(TcpMessage_0x20));

        // JSON-данные
        if (jsonSize > 0) {
            buffer.append(jsonData, static_cast<int>(jsonSize));
        }

        return buffer;
    }

/// @warning Отключение диагностики комплилятора для метода копирования
/// @note Мы сами перепроверяем строку при её сериализации/десериализации
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
    /**
     * @brief Десериализация из массива байт
     * @param data Входные данные
     * @return Экземпляр сообщения
     */
    static TcpMessage_0x20 deserialize(const QByteArray& data) {
        TcpMessage_0x20 msg;

        // Копирование заголовка
        const size_t headerSize = sizeof(TcpMessage_0x20);
        memcpy(&msg, data.constData(), headerSize);

        // Копирование JSON
        const size_t jsonSize = msg.getJsonSize();
        if (jsonSize > 0) {
            msg.jsonData = new char[jsonSize + 1];
            memcpy(msg.jsonData, data.constData() + headerSize, jsonSize);
            msg.jsonData[jsonSize] = '\0';
        }

        return msg;
    }
#pragma GCC diagnostic pop

    std::string toString() const {
        std::ostringstream oss;
        oss << TcpHeader::toString() << "\n"
            << "Module Scheme (0x20):\n"
            << "  JSON Data Size: " << getJsonSize() << " bytes\n";

        try {
            auto elements = getElements();
            oss << "  Elements Count: " << elements.size();

            // Выводим только первые 3 элемента для краткости
            for (size_t i = 0; i < std::min(elements.size(), size_t(3)); ++i) {
                const auto& elem = elements[i];
                oss << "\n\n  Element #" << i << ":\n"
                    << "    ID: " << elem.IDElem << "\n"
                    << "    Name: " << elem.Name << "\n"
                    << "    Short Name: " << elem.ShortName << "\n"
                    << "    Sub-elements: " << elem.Elements.size() << "\n"
                    << "    Parameters: " << elem.Params.size() << "\n"
                    << "    Config Params: " << elem.ConfigParams.size();
            }

            if (elements.size() > 3) {
                oss << "\n  ... and " << (elements.size() - 3)
                    << " more elements";
            }
        } catch (const std::exception& e) {
            oss << "\n  Failed to parse JSON: " << e.what();
        }
        return oss.str();
    }

private:
    /**
     * @brief Инициализация из JSON-объекта
     * @param jsonObj JSON-объект
     */
    void initializeFromJson(const json& jsonObj) {
        const std::string jsonStr = jsonObj.dump();
        const size_t jsonSize = jsonStr.size();

        jsonData = new char[jsonSize + 1];
        memcpy(jsonData, jsonStr.c_str(), jsonSize);
        jsonData[jsonSize] = '\0';

        sizeData = sizeof(TcpMessage_0x20) - sizeof(TcpHeader) + jsonSize;
    }

    /**
     * @brief Получение размера JSON-данных
     * @return Размер данных
     */
    size_t getJsonSize() const {
        return sizeData - (sizeof(TcpMessage_0x20) - sizeof(TcpHeader));
    }

    TcpMessage_0x20() : jsonData(nullptr) {} // Для десериализации
};
#pragma pack(pop)

static_assert(sizeof(TcpMessage_0x20) == 16 + sizeof(char*),
              "Invalid TcpMessage_0x20 size");

#pragma pack(push, 1)

/**
 * @struct ElementStatus
 * @brief Статус одного элемента схемы
 */
struct ElementStatus {
    uint16_t IDElem;    ///< Условный номер элемента (0-65535)

    uint8_t status:4;   ///< Состояние элемента (0-неизвестно, 1-норма,
                        ///< 2-предупреждение, 3-авария)
    uint8_t workFl:4;   ///< Статус работы (0-выключен, 1-включен, 2-включение,
                        ///< 3-выключение)

    uint8_t local:1;    ///< Признак местного режима (0/1)
    uint8_t isImit:1;   ///< Признак имитатора (0/1)
    uint8_t reserv:6;   ///< Резервные биты
};

/**
 * @struct TcpMessage_0x21_Header
 * @brief Основная структура сообщения (только заголовок) (0x21)
 */
struct TcpMessage_0x21_Header : TcpMessage<TcpMessage_0x21_Header,
                                PBU_MessageNumbers::STATUS_SCHEME_ELEMENTS> {
    uint32_t time1;
    uint32_t time2;

    TcpMessage_0x21_Header() {
        uint64_t time = to_uint64(TimeProvider::instance().getMs());
        time1 = to_uint32(time & 0xFFFFFFFF);
        time2 = to_uint32(time >> 32);
    }
};


/**
  * @class TcpMessage_0x21
  * @brief Сообщение статуса элементов схемы (0x21)
  * @note Обертка для удобной работы
  */
class TcpMessage_0x21 {
    TcpMessage_0x21_Header header;
    std::vector<ElementStatus> elements;

public:

    /// @brief Конструкторы
    /// @{
    TcpMessage_0x21() = default;

    explicit TcpMessage_0x21(const std::vector<ElementStatus>& elems)
        : elements(elems) {
        updateSize();
    }
    /// @}

    /// @brief Управление элементами
    /// @{
    void setElements(const std::vector<ElementStatus>& elems) {
        elements = elems;
        updateSize();
    }

    void addElement(const ElementStatus& elem) {
        elements.push_back(elem);
        updateSize();
    }

    const std::vector<ElementStatus>& getElements() const { return elements; }
    ///@}

    /// @brief Сериализация
    QByteArray serializeFull() const {
        QByteArray data;
        data.append(reinterpret_cast<const char*>(&header), sizeof(header));
        data.append(reinterpret_cast<const char*>(elements.data()),
                    elements.size() * sizeof(ElementStatus));
        return data;
    }

    /// @brief Десериализация
    static TcpMessage_0x21 deserialize(const QByteArray& data) {
        TcpMessage_0x21 msg;

        if (data.size() >= static_cast<int>(sizeof(TcpMessage_0x21_Header))) {
            /// Читаем заголовок
            memcpy(&msg.header, data.constData(),
                   sizeof(TcpMessage_0x21_Header));

            /// Читаем элементы
            const size_t elementsSize = (data.size() -
                                         sizeof(TcpMessage_0x21_Header)) /
                                        sizeof(ElementStatus);
            msg.elements.resize(elementsSize);

            if (elementsSize > 0) {
                memcpy(msg.elements.data(),
                       data.constData() + sizeof(TcpMessage_0x21_Header),
                       elementsSize * sizeof(ElementStatus));
            }
        }

        return msg;
    }

    std::string toString() const {
        std::ostringstream oss;
        oss << "Status of Scheme Elements (0x21):\n"
            << "  Elements Count: " << elements.size() << "\n";

        // Выводим только первые 5 элементов для краткости
        for (size_t i = 0; i < std::min(elements.size(), size_t(5)); ++i) {
            const auto& elem = elements[i];
            oss << "\n  Element #" << i << " (ID: " << elem.IDElem << "):\n"
                << "    Status: ";

            switch (elem.status) {
            case 0: oss << "Unknown"; break;
            case 1: oss << "Normal"; break;
            case 2: oss << "Warning"; break;
            case 3: oss << "Failure"; break;
            default: oss << "Invalid"; break;
            }

            oss << "\n    Work State: ";
            switch (elem.workFl) {
            case 0: oss << "Off"; break;
            case 1: oss << "On"; break;
            case 2: oss << "Starting"; break;
            case 3: oss << "Stopping"; break;
            default: oss << "Invalid"; break;
            }

            oss << "\n    Local Mode: " << (elem.local ? "Yes" : "No")
                << "\n    Is Simulated: " << (elem.isImit ? "Yes" : "No");
        }

        if (elements.size() > 5) {
            oss << "\n  ... and " << (elements.size() - 5) << " more elements";
        }

        return oss.str();
    }

private:
    void updateSize() {
        header.sizeData = sizeof(TcpMessage_0x21_Header) - sizeof(TcpHeader) +
                          elements.size() * sizeof(ElementStatus);
    }
};

#pragma pack(pop)

static_assert(sizeof(ElementStatus) == 4, "ElementStatus size mismatch");
static_assert(sizeof(TcpMessage_0x21_Header) == 16 + 8,
              "TcpMessage_0x21_Header size mismatch");

#pragma pack(push, 1)
/**
 * @struct TcpMessage_0x23
 * @brief Квитанция на получение команды управления
 */
struct TcpMessage_0x23: TcpMessage<TcpMessage_0x23,
                                    PBU_MessageNumbers::RECEIPT_CONTROL_COMMAND> {
    /// @name Метка времени
    /// @details В данное поле записывается UTC время ПО модуля на момент
    /// формирования квитанции на команду.
    /// @note Передается в виде 64-битного значения кол-ва миллисекунд,
    /// прошедших с 00:00:00.000 1970 01 01 (Unix Epoch)
    /// @{
    uint32_t time1;     ///< @brief Младшие 32 бита временной метки

    uint32_t time2;     ///< @brief Старшие 32 бита временной метки
    /// @}

    uint16_t IDCom;     ///< @brief Идентификатор команды
                        ///< @details В качестве идентификатора команды
                        ///< используется номер передаваемого пакета из общего
                        ///< заголовка принятой команды

    uint16_t code;      ///< @brief Код выполнения команды
                        ///< @details Коды выполнения от 0 до 127 включительно
                        ///< общие для любой команды для всех модулей:
                        ///< - "0": команда выполнена без ошибок;
                        ///< - "1": команда в процессе выполнения;
                        ///< - "2": некорректные параметры — передается в случае
                        ///< некорректности хотя бы одного из параметров;
                        ///< - "3": выполнение команды прервано новой командой
                        ///< управления либо отменено оператором;
                        ///< - "4": команда не выполнена по причине
                        ///< неисправности оборудования;
                        ///< - "5": команда не выполнена из-за внутренней
                        ///< ошибки;
                        ///< - "6": неизвестный тип сообщения;
                        ///< - "7": команда отклонена, так как РАФ выключен;
                        ///< - "8": команда не выполнена по причине неготовности
                        ///< оборудования;
                        ///< - "9": команда не выполнена, так как модуль
                        ///< находится в местном режиме;
                        ///< - "10": команда не выполнена, так как модуль
                        ///< находится в РАФ и/или в РГДВ;
                        ///< - "11": команда не выполнена, так как модуль
                        ///< находится в ТП;
                        ///< - "127": неизвестная ошибка.
                        ///< @note Коды с 128 до 255 включительно — коды ошибок
                        ///< для определенной команды одинаковые для разных
                        ///< типов подключаемых модулей:
                        ///<    - "128": требуемый язык не поддерживается;
                        ///<    - "129": отмена установки времени по причине
                        ///<    исправности собственного источника времени;
                        ///<    - "130": ошибка сохранения.
                        ///< @warning Коды с 256 по 65535 специфичные для
                        ///< каждого модуля, определяются в унифицированном
                        ///< протоколе сопряжения ПО модуля с составными частями
                        ///< РУК КББ (приложение 1 к протоколу)

    TcpMessage_0x23()
        : IDCom(AppConfig::instance().PBU_PROTO_0X23_ID_COM()),
        code(AppConfig::instance().PBU_PROTO_0X23_CODE()) {
        uint64_t time = to_uint64(TimeProvider::instance().getMs());
        time1 = to_uint32(time & 0xFFFFFFFF);
        time2 = to_uint32(time >> 32);
    }

    std::string toString() const {
        std::ostringstream oss;
        uint64_t time = (static_cast<uint64_t>(time2) << 32) | time1;
        oss << TcpHeader::toString() << "\n"
            << "Command Receipt (0x23):\n"
            << "  Time: " << TimeProvider::instance().msToDateTime(time) << "\n"
            << "  Command ID: " << IDCom << "\n"
            << "  Result Code: " << static_cast<int>(code) << " (";

        // Расшифровка кода
        switch (code) {
        case 0: oss << "Success"; break;
        case 1: oss << "In progress"; break;
        case 2: oss << "Invalid parameters"; break;
        case 3: oss << "Cancelled"; break;
        case 4: oss << "Hardware failure"; break;
        case 5: oss << "Internal error"; break;
        case 6: oss << "Unknown message type"; break;
        case 7: oss << "RAF is off"; break;
        case 8: oss << "Not ready"; break;
        case 9: oss << "Local mode"; break;
        case 10: oss << "In RAF/RDGV mode"; break;
        case 11: oss << "In transport position"; break;
        case 127: oss << "Unknown error"; break;
        case 128: oss << "Language not supported"; break;
        case 129: oss << "Time sync cancelled"; break;
        case 130: oss << "Save error"; break;
        case 256: oss << "Target not found"; break;
        case 257: oss << "Out of coverage"; break;
        default:
            if (code >= 256) oss << "Module-specific error";
            else oss << "Unknown code";
            break;
        }
        oss << ")";
        return oss.str();
    }
};
#pragma pack(pop)

static_assert(sizeof(TcpMessage_0x23) == 16 + 12,
              "TcpMessage_0x23 size mismatch");

#pragma pack(push, 1)
/**
 * @struct TcpMessage_0x24
 * @brief Статус модуля
 */
struct TcpMessage_0x24: TcpMessage<TcpMessage_0x24,
                                    PBU_MessageNumbers::STATUS_MODULE> {
    /// @name Метка времени
    /// @details В данное поле записывается UTC время ПО модуля на момент
    /// формирования сообщения.
    /// @note Передается в виде 64-битного значения кол-ва миллисекунд,
    /// прошедших с 00:00:00.000 1970 01 01 (Unix Epoch)
    /// @{
    uint32_t time1;         ///< @brief Младшие 32 бита временной метки

    uint32_t time2;         ///< @brief Старшие 32 бита временной метки
    /// @}

    uint8_t status: 3;      ///< @brief Состояние модуля
                            ///< @details Данное поле может принимать следующие
                            ///< значения:
                            ///< - "0": неизвестно
                            ///< - "1": норма
                            ///< - "2": предупреждение
                            ///< - "3": авария.

    uint8_t work: 3;        ///< @brief Статус работы
                            ///< @details Данное поле может принимать следующие
                            ///< значения:
                            ///< - "0": выключен
                            ///< - "1": включен
                            ///< - "2": включение
                            ///< - "3": выключение.

    uint8_t isRGDV: 1;      ///< @brief Признак включения РГДВ
                            ///< @details В данном поле передается «1»,
                            ///< если модуль находится в РГДВ, «0» - если нет
                            ///< default - 1

    uint8_t isRAF: 1;       ///< @brief Признак включения РАФ
                            ///< @details В данном поле передается «1», если
                            ///< модуль находится в РАФ, «0» - если нет
                            ///< default - 1

    uint8_t isLocal: 1;     ///< @brief Признак включения местного режима
                            ///< @details В данном поле передается «1», если
                            ///< модуль находится в местном режиме управления,
                            ///< «0» - если нет.
                            ///< @warning В режиме местного управления модуль
                            ///< игнорирует все команды, приходящие от ПО
                            ///< Сервера. Переход в местный режим и выход из
                            ///< него возможен только с технологического
                            ///< рабочего места оператора модуля.
                            ///< default - 0

    uint8_t isImit: 1;      ///< @brief Признак имитатора
                            ///< @details В данном поле передается «1», если
                            ///< модуль находится в режиме имитатора,
                            ///< «0» - если нет.

    uint8_t hasTP: 1;       ///< @brief Признак наличия транспортного положения
                            ///< @details В данном поле передается признак
                            ///< наличия у модуля специального положения
                            ///< оборудования для транспортировки комплекса.
                            ///< @note Значение 1 передается в том случае, если:
                            ///< - у модуля есть транспортное положение,
                            ///< - нахождение модуля в транспортном положении
                            ///< контролируется из ПО,
                            ///< - возможен переход в транспортное положение и
                            ///< выход из него по команде от ПО Сервера.
                            ///< Если не выполнено хотя бы одно условие, в
                            ///< данном поле передается 0.
                            ///< @warning Если в поле Признак наличия
                            ///< транспортного положения передается 0, то поля
                            ///< Признак нахождения в ТП, Признак нахождения в
                            ///< РП, Признак достоверности флага ТП, Признак
                            ///< достоверности флага РП и Статус положения
                            ///< игнорируются.

    uint8_t isTP: 1;        ///< @brief Признак нахождения в транспортном
                            ///< положении
                            ///< @details В данном поле передается признак
                            ///< нахождения модуля в транспортном положении:
                            ///< - "0": модуль не находится в транспортном
                            ///< положении — не выполнено хотя бы одно из
                            ///< условий нахождения в ТП или нет возможности
                            ///< определить нахождение модуля в ТП (нет связи с
                            ///< устройством),
                            ///< - "1": модуль находится в транспортном
                            ///< положении — выполнены все условия нахождения
                            ///< модуля в ТП.
                            ///< @warning Если в поле Признак наличия
                            ///< транспортного положения передается 0, то данное
                            ///< поле игнорируется.

    uint8_t isWP: 1;        ///< @brief Признак нахождения в РП
                            ///< @details В данном поле передается признак
                            ///< нахождения модуля в рабочем положении:
                            ///< - 0: модуль не находится в рабочем положении —
                            ///< не выполнено хотя бы одно из условий нахождения
                            ///< в РП или нет возможности определить нахождение
                            ///< модуля в РП (нет связи с устройством),
                            ///< - 1: модуль находится в рабочем положении —
                            ///< выполнены все условия нахождения модуля в РП.

    uint8_t isTPValid: 1;   ///< @brief Признак достоверности флага ТП
                            ///< @details В данном поле передается достоверность
                            ///< флага ТП:
                            ///< - 0: не удалось определить нахождение модуля
                            ///< в ТП (нет ответа от устройств);
                            ///< - 1: признак нахождения модуля в ТП достоверен.

    uint8_t isWPValid: 1;   ///< @brief Признак достоверности флага РП
                            ///< @details В данном поле передается достоверность
                            ///< флага РП:
                            ///< - 0: не удалось определить нахождение модуля в
                            ///< РП (нет ответа от устройств);
                            ///< - 1: признак нахождения модуля в РП достоверен.

    uint8_t statusTWP: 1;   ///< @brief Статус положения
                            ///< @details В данном поле передается статус
                            ///< положения модуля:
                            ///< - "0": авария;
                            ///< - "1": норма.
                            ///< @warning При выставлении аварии в данном поле,
                            ///< статус модуля должен быть **авария**
                            ///< @note Статус положения выставляется в
                            ///< «0» - «авария» в случаях:
                            ///< - не удалось определить нахождение модуля в ТП
                            ///< и/или РП;
                            ///< - модуль находится в запрещенном положении;
                            ///< - после поступления команды «Перейти в ТП»
                            ///< модуль не перешел в ТП и признак достоверности
                            ///< равен «1»;
                            ///< - после поступления команды «Выйти из ТП
                            ///< (Перейти в РП)» модуль не перешел в РП и
                            ///< признак достоверности равен «1».
                            ///< \e В остальных случаях статус положения
                            ///< выставляется в «1» - «норма».

    uint16_t mode;          ///< @brief Режим функционирования
                            ///< @details В данное поле передается текущий режим
                            ///< функционирования модуля:
                            ///< - "0": ожидание команд управления;
                            ///< - "1": юстировка.
                            ///< @warning Номера с 256 по 65535 специфичные для
                            ///< каждого модуля, определяются в унифицированном
                            ///< протоколе сопряжения ПО модуля с составными
                            ///< частями РУК КББ (приложение 1 к протоколу)

    uint32_t reserve;       ///< @brief Резерв (не используется)

    TcpMessage_0x24()
        : status(AppConfig::instance().PBU_PROTO_0X24_STATUS()),
        work(AppConfig::instance().PBU_PROTO_0X24_WORK()),
        isRGDV(AppConfig::instance().PBU_PROTO_0X24_IS_RGDV()),
        isRAF(AppConfig::instance().PBU_PROTO_0X24_IS_RAF()),
        isLocal(AppConfig::instance().PBU_PROTO_0X24_IS_LOCAL()),
        isImit(AppConfig::instance().PBU_PROTO_0X24_IS_IMIT()),
        hasTP(AppConfig::instance().PBU_PROTO_0X24_HAS_TP()),
        isTP(AppConfig::instance().PBU_PROTO_0X24_IS_TP()),
        isWP(AppConfig::instance().PBU_PROTO_0X24_IS_WP()),
        isTPValid(AppConfig::instance().PBU_PROTO_0X24_IS_TP_VALID()),
        isWPValid(AppConfig::instance().PBU_PROTO_0X24_IS_WP_VALID()),
        statusTWP(AppConfig::instance().PBU_PROTO_0X24_STATUS_TWP()),
        mode(AppConfig::instance().PBU_PROTO_0X24_MODE()),
        reserve(0) {
        uint64_t time = TimeProvider::instance().getTimeForRequest();
        time1 = to_uint32(time & 0xFFFFFFFF);
        time2 = to_uint32(time >> 32);
    }

    std::string toString() const {
        std::ostringstream oss;
        uint64_t time = (static_cast<uint64_t>(time2) << 32) | time1;
        oss << TcpHeader::toString() << "\n"
            << "Module Status (0x24):\n"
            << "  Time: " << TimeProvider::instance().msToDateTime(time) << "\n"
            << "  Status: ";

        switch (status) {
        case 0: oss << "Unknown"; break;
        case 1: oss << "Normal"; break;
        case 2: oss << "Warning"; break;
        case 3: oss << "Failure"; break;
        default: oss << "Invalid"; break;
        }

        oss << "\n  Work State: ";
        switch (work) {
        case 0: oss << "Off"; break;
        case 1: oss << "On"; break;
        case 2: oss << "Starting"; break;
        case 3: oss << "Stopping"; break;
        default: oss << "Invalid"; break;
        }

        oss << "\n  RDGV Mode: " << (isRGDV ? "Yes" : "No") << "\n"
            << "  RAF Mode: " << (isRAF ? "Yes" : "No") << "\n"
            << "  Local Control: " << (isLocal ? "Yes" : "No") << "\n"
            << "  Simulation Mode: " << (isImit ? "Yes" : "No") << "\n"
            << "  Transport Supported: " << (hasTP ? "Yes" : "No") << "\n";

        if (hasTP) {
            oss << "  In Transport Position: " << (isTP ? "Yes" : "No") << "\n"
                << "  In Working Position: " << (isWP ? "Yes" : "No") << "\n"
                << "  TP Status Valid: " << (isTPValid ? "Yes" : "No") << "\n"
                << "  WP Status Valid: " << (isWPValid ? "Yes" : "No") << "\n"
                << "  Position Status: " << (statusTWP ? "Normal" : "Failure");
        }

        oss << "\n  Operation Mode: ";
        switch (mode) {
        case 0: oss << "Standby"; break;
        case 1: oss << "Calibration"; break;
        default: oss << "Custom (" << mode << ")"; break;
        }

        return oss.str();
    }
};
#pragma pack(pop)

static_assert(sizeof(TcpMessage_0x24) == 16 + 16,
              "TcpMessage_0x24 size mismatch");

// -----------------------------------------
// Сообщения от ПБУ на ВУ [КОНТРОЛЯ И УПРАВЛЕНИЯ]
// -----------------------------------------

#pragma pack(push, 1)
/**
 * @struct TcpMessage_0x40
 * @brief Команда «Включить»
 */
struct TcpMessage_0x40: TcpMessage<TcpMessage_0x40,
                                    PBU_MessageNumbers::COMMAND_SWITCH_ON> {
    uint32_t flags; ///< @brief Флаги включения
                    ///< @details . Данное поле может принимать следующие
                    ///< значения:
                    ///< - "0": включить оборудование;
                    ///< - "1": включить режим готовности дистанционного
                    ///< включения (РГДВ);
                    ///< - "2": включить режим автоматического функционирования
                    ///< (РАФ);
                    ///< - "3": выйти из транспортного положения.
                    ///< @note <b>Примечание 1</b>
                    ///< Команда с флагом 3 «выйти из транспортного положения»
                    ///< должна отправляться до включения РГДВ. Если при
                    ///< получении команды с данным флагом модуль находится в
                    ///< РАФ и/или РГДВ, то команда игнорируется и посылается
                    ///< квитанция с кодом 10 «команда не выполнена, так как
                    ///< модуль находится в РАФ и/или в РГДВ».
                    ///< @note <b>Примечание 2</b>
                    ///< Если модуль находится в транспортном положении, то
                    ///< команда с флагом 1 (включить РГДВ) и 2 (включить РАФ)
                    ///< должна игнорироваться, в ответ должна посылаться
                    ///< квитанция с кодом 11 - «команда не выполнена, так как
                    ///< модуль находится в ТП»
                    ///< @warning При получении данной команды ПО модуля должно
                    ///< выполнить включение соответствующих режимов и отправить
                    ///< квитанцию о выполнении команды в ПО Сервера:
                    ///<    - "0": команда выполнена без ошибок;
                    ///<    - "1": команда в процессе выполнения;
                    ///<    - "2": некорректные параметры — передается в случае
                    ///<    некорректности хотя бы одного из параметров;
                    ///<    - "3": выполнение команды прервано новой командой
                    ///<    управления либо отменено оператором;
                    ///<    - "4": команда не выполнена по причине неисправности
                    ///<    оборудования;
                    ///<    - "5": команда не выполнена из-за внутренней ошибки;
                    ///<    - "8": команда не выполнена по причине неготовности
                    ///<    оборудования;
                    ///<    - "9": команда не выполнена, так как модуль
                    ///<    находится в местном режиме;
                    ///<    - "127": неизвестная ошибка.

    std::string toString() const {
        std::ostringstream oss;
        oss << TcpHeader::toString() << "\n"
            << "Switch On Command (0x40):\n"
            << "  Command: ";

        switch (flags) {
        case 0: oss << "Turn equipment on"; break;
        case 1: oss << "Enable RDGV mode"; break;
        case 2: oss << "Enable RAF mode"; break;
        case 3: oss << "Exit transport position"; break;
        default: oss << "Unknown command (" << flags << ")"; break;
        }

        return oss.str();
    }
};
#pragma pack(pop)

static_assert(sizeof(TcpMessage_0x40) == 16 + 4,
              "TcpMessage_0x40 size mismatch");

#pragma pack(push, 1)
/**
 * @struct TcpMessage_0x41
 * @brief Команда «Выключить»
 */
struct TcpMessage_0x41: TcpMessage<TcpMessage_0x41,
                                    PBU_MessageNumbers::COMMAND_SWITCH_OFF> {
    uint32_t flags; ///< @brief Флаги включения
                    ///< @details Данное поле может принимать следующие
                    ///< значения:
                    ///< - "0": выключить оборудование;
                    ///< - "1": выключить режим автоматического функционирования
                    ///< (РАФ);
                    ///< - "2": выключить РГДВ;
                    ///< - "3": выключить все оборудование и ПО модуля
                    ///< - "4": выключить все оборудование и ПК модуля
                    ///< - "5": перейти в транспортное положение
                    ///< @note <b>Примечание</b>
                    ///< Команда с флагом 5 «перейти в транспортное положение»
                    ///< должна отправляться до включения или после выключения
                    ///< РГДВ. Если при получении команды с данным флагом модуль
                    ///< находится в РАФ и/или РГДВ, то команда игнорируется и
                    ///< посылается квитанция с кодом 10 «команда не выполнена,
                    ///< так как модуль находится в РАФ и/или в РГДВ»
                    ///< @warning При получении данной команды ПО модуля должно
                    ///< отправить квитанцию о выполнении команды в ПО Сервера и
                    ///< выполнить выключение:
                    ///<    - "0": команда выполнена без ошибок;
                    ///<    - "1": команда в процессе выполнения;
                    ///<    - "2": некорректные параметры — передается в случае
                    ///<    некорректности хотя бы одного из параметров;
                    ///<    - "3": выполнение команды прервано новой командой
                    ///<    управления либо отменено оператором;
                    ///<    - "4": команда не выполнена по причине неисправности
                    ///<    оборудования;
                    ///<    - "5": команда не выполнена из-за внутренней ошибки;
                    ///<    - "8": команда не выполнена по причине неготовности
                    ///<    оборудования;
                    ///<    - "9": команда не выполнена, так как модуль
                    ///<    находится в местном режиме;
                    ///<    - "127": неизвестная ошибка.

    std::string toString() const {
        std::ostringstream oss;
        oss << TcpHeader::toString() << "\n"
            << "Switch Off Command (0x41):\n"
            << "  Command: ";

        switch (flags) {
        case 0: oss << "Turn equipment off"; break;
        case 1: oss << "Disable RAF mode"; break;
        case 2: oss << "Disable RDGV mode"; break;
        case 3: oss << "Shutdown module software and hardware"; break;
        case 4: oss << "Shutdown module PC"; break;
        case 5: oss << "Enter transport position"; break;
        default: oss << "Unknown command (" << flags << ")"; break;
        }

        return oss.str();
    }
};
#pragma pack(pop)

static_assert(sizeof(TcpMessage_0x41) == 16 + 4,
              "TcpMessage_0x41 size mismatch");

#pragma pack(push, 1)
/**
 * @struct TcpMessage_0x41
 * @brief Команда «Сброс аварии»
 * @details Данная команда отправляется ПО Сервера для сброса аварии элемента
 * схемы (и всех вложенных в него элементов, находящихся в аварии),
 * идентификатор которого указан в поле "Идентификатор элемента"
 * @note При получении данной команды ПО модуля должно выполнить сброс аварии и
 * отправить квитанцию о выполнении команды в ПО Сервера:
 * - "0": команда выполнена без ошибок;
 * - "1": команда в процессе выполнения;
 * - "2": некорректные параметры — передается в случае некорректности хотя бы
 * одного из параметров;
 * - "3": выполнение команды прервано новой командой управления либо отменено
 * оператором;
 * - "4": команда не выполнена по причине неисправности оборудования;
 * - "5": команда не выполнена из-за внутренней ошибки;
 * - "8": команда не выполнена по причине неготовности оборудования;
 * - "9": команда не выполнена, так как модуль находится в местном режиме;
 * - "127": неизвестная ошибка.
 */
struct TcpMessage_0x42: TcpMessage<TcpMessage_0x42,
                                    PBU_MessageNumbers::COMMAND_RESET_ALARM> {
    uint16_t IDElem;    ///< @brief Идентификатор элемента

    uint16_t reserve;   ///< @brief Резервное поле (Не используется)

    std::string toString() const {
        std::ostringstream oss;
        oss << TcpHeader::toString() << "\n"
            << "Reset Alarm Command (0x42):\n"
            << "  Element ID: " << IDElem << "\n"
            << "  Reserved: " << reserve;
        return oss.str();
    }
};
#pragma pack(pop)

static_assert(sizeof(TcpMessage_0x42) == 16 + 4,
              "TcpMessage_0x42 size mismatch");

/**
 * @class MessageFactory
 * @brief Фабрика для создания и парсинга сообщений
 */
class MessageFactory {
public:
    /**
     * @brief Создание сообщения указанного типа (по-умолчанию)
     * @tparam T Тип сообщения (должен наследовать от TcpHeader)
     * @return Экземпляр сообщения
     */
    template<typename T>
    static typename std::enable_if<std::is_base_of<TcpHeader, T>::value, T>::
        type create() {
        return T();
    }
    /**
     * @brief Создание сообщения с произвольными параметрами
     * @tparam T Тип сообщения (должен наследовать от TcpHeader)
     * @tparam... Args Произвольные аргументы для конструкторов сообщений,
     * которым необходимы параметры
     * @return Экземпляр сообщения
     */
    template<typename T, typename... Args>
    static typename std::enable_if<std::is_base_of<TcpHeader, T>::value, T>::
        type create(Args&&... args) {
        return T(std::forward<Args>(args)...);
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
            case PBU_MessageNumbers::RES_REGISTRATION: {
                return parseMessage<TcpMessage_0x02>(data);
            }
            case PBU_MessageNumbers::TIMESTAMP: {
                return parseMessage<TcpMessage_0x04>(data);
            }
            case PBU_MessageNumbers::TARGET_DESIGNATION: {
                return parseMessage<TcpMessage_0x641>(data);
            }
            // case PBU_MessageNumbers::GEO_COMPLEX: {
            //     return parseMessage<TcpMessage_0x06>(data)
            // }
            // case PBU_MessageNumbers::MARK_TRAICE_PBU: {

            // }
            // case PBU_MessageNumbers::ORIENTATION_SC_COMPLEX: {

            // }

            // case PBU_MessageNumbers::COMMAND_SWITCH_ON: {

            // }
            // case PBU_MessageNumbers::COMMAND_SWITCH_OFF: {

            // }
            // case PBU_MessageNumbers::COMMAND_RESTART: {

            // }
            // case PBU_MessageNumbers::COMMAND_RESET_ALARM: {

            // }
            // case PBU_MessageNumbers::COMMAND_SET_CORRECTIONS: {

            // }
            // case PBU_MessageNumbers::REQ_MODULE_SCHEME: {

            // }
            // case PBU_MessageNumbers::COMMAND_SET_LANGUAGE: {

            // }
            // case PBU_MessageNumbers::MESSAGE_SATELLITE_NAV: {

            // }
            // case PBU_MessageNumbers::COMMAND_SET_TIME: {

            // }
            // case PBU_MessageNumbers::COMMAND_SET_COORDINATES_MODULE: {

            // }
            // case PBU_MessageNumbers::COMMAND_SET_SETTINGS_VALUES: {

            // }
            // Обработка других типов...
            default: return nullptr;
            }
    }

    static std::string messageToString(std::unique_ptr<TcpHeader>& msg) {
        switch (static_cast<PBU_MessageNumbers>(msg->typePack)) {
        case PBU_MessageNumbers::REQ_REGISTRATION:
            return reinterpret_cast<TcpMessage_0x01*>(msg.get())->toString();
        case PBU_MessageNumbers::RES_REGISTRATION:
            return reinterpret_cast<TcpMessage_0x02*>(msg.get())->toString();
        case PBU_MessageNumbers::REQ_TIME:
            return reinterpret_cast<TcpMessage_0x03*>(msg.get())->toString();
        case PBU_MessageNumbers::TIMESTAMP:
            return reinterpret_cast<TcpMessage_0x04*>(msg.get())->toString();
        case PBU_MessageNumbers::GEO_MODULE:
            return reinterpret_cast<TcpMessage_0x05*>(msg.get())->toString();
        case PBU_MessageNumbers::CURRENT_STATE_RLS:
            return reinterpret_cast<TcpMessage_0x1021*>(msg.get())->toString();
        case PBU_MessageNumbers::MARK_TARGET:
            return reinterpret_cast<TcpMessage_0x1022*>(msg.get())->toString();
        case PBU_MessageNumbers::MODULE_CAPABILITIES:
            return reinterpret_cast<TcpMessage_0x1023*>(msg.get())->toString();
        case PBU_MessageNumbers::FORBIDDEN_SECTORS:
            return reinterpret_cast<TcpMessage_0x1024*>(msg.get())->toString();
        case PBU_MessageNumbers::MARK_TRAICE_PBU:
            return reinterpret_cast<TcpMessage_0x07*>(msg.get())->toString();
        case PBU_MessageNumbers::TARGET_DESIGNATION:
            return reinterpret_cast<TcpMessage_0x641*>(msg.get())->toString();
        case PBU_MessageNumbers::MODULE_SCHEME:
            return reinterpret_cast<TcpMessage_0x20*>(msg.get())->toString();
        case PBU_MessageNumbers::STATUS_SCHEME_ELEMENTS:
            return reinterpret_cast<TcpMessage_0x21*>(msg.get())->toString();
        case PBU_MessageNumbers::RECEIPT_CONTROL_COMMAND:
            return reinterpret_cast<TcpMessage_0x23*>(msg.get())->toString();
        case PBU_MessageNumbers::STATUS_MODULE:
            return reinterpret_cast<TcpMessage_0x24*>(msg.get())->toString();
        case PBU_MessageNumbers::COMMAND_SWITCH_ON:
            return reinterpret_cast<TcpMessage_0x40*>(msg.get())->toString();
        case PBU_MessageNumbers::COMMAND_SWITCH_OFF:
            return reinterpret_cast<TcpMessage_0x41*>(msg.get())->toString();
        case PBU_MessageNumbers::COMMAND_RESET_ALARM:
            return reinterpret_cast<TcpMessage_0x42*>(msg.get())->toString();
        default:
            return "Unsupported message type: 0x" +
                   QString::number(msg->typePack, 16).toStdString();
        }
    }

private:
    /**
     * @brief Парсит сообщение типа T из данных
     * @private
     * @tparam T Тип сообщения (наследник TcpHeader)
     * @param data Входные данные
     * @return Уникальный указатель на сообщение или nullptr при ошибке
     */
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
/**
 * @example Пример использования фабрики сообщений
 * @code
 * // Создание сообщения
 * auto msg = MessageFactory::create<TcpMessage_0x01>();
 * QByteArray data = msg.serializeFull();
 *
 * // Парсинг
 * auto parsed = MessageFactory::parse(data);
 *
 * // Используем reinterpret_cast, а не dynamic_cast потому что
 * // реализация unique_ptr не позволяет повторять создание
 * // указателя на память ранее используемую в контексте исполнения
 * if(auto msg01 = reinterpret_cast<TcpMessage_0x01>(parsed.get())) {
 *    // Работа с сообщением...
 * }
 * @endcode
 */

#endif // TCPMESSAGE_H
