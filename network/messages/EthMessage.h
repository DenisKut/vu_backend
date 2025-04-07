#ifndef ETHMESSAGE_H
#define ETHMESSAGE_H

#pragma once
#include <QByteArray>
#include <type_traits>

struct Message {
    quint64 timestamp;
    QByteArray payload;
};

// Преобразование enum class в базовый тип
template <typename E>
constexpr auto to_underlying(E e) -> typename std::underlying_type<E>::type {
    return static_cast<typename std::underlying_type<E>::type>(e);
}

// =========================================
// Номера сообщений по протоколу связи с БС
// =========================================
/**
 * @enum ETH_MessageNumbers
 * @brief Перечисление типов сообщений протоколов БС, ППУ и МЦФОС
 */
enum class ETH_MessageNumbers: uint16_t {
    // -----------------------------------------
    // Сообщения от БС на ВУ [ВРЕМЯ]
    // -----------------------------------------
    BS_SYNC             = 0x0205,    ///< 2.5   - сообщение синхронизации БС;

    // -----------------------------------------
    // Сообщения от БС на ВУ [ПРИВОД]
    // -----------------------------------------
    PPU_POSITION        = 0x0B01,    ///< 11.1  - информация ориентации ППУ;

    // -----------------------------------------
    // Сообщения от ВУ на БС [ПРИВОД]
    // -----------------------------------------
    PPU_STOP            = 0x0B04,    ///< 11.4  – команда остановки ППУ;
    PPU_ENABLE          = 0x0B06,    ///< 11.6  - команда включения ППУ;
    PPU_MOVE            = 0x0B08,    ///< 11.8  - целеуказание для ППУ;

    // -----------------------------------------
    // Сообщения от БС на ВУ [МЦФОС]
    // -----------------------------------------
    MCFS_RESULT         = 0x0F01,    ///< 15.1  - информация по обнаруженным объектам;
    MCFS_REG            = 0x0F03,    ///< 15.3  - информация регистрации;
    MCFS_STATUS         = 0x0C0B,    ///< 12.11 - состояние устройства;

    // -----------------------------------------
    // Сообщения от ВУ на БС [МЦФОС]
    // -----------------------------------------
    MCFS_SGNL_GEN       = 0x0C01,    ///< 12.1  - генерация сигнала МЦФОС;
    MCFS_SGNL_REC       = 0x0C02,    ///< 12.2  - приём сигнала МЦФОС;
    MCFS_SGNL_GEN_REC   = 0x0C03,    ///< 12.3  - генерация и приём сигнала МЦФОС;

    // -------НЕ ИСПОЛЬЗОВАТЬ-------
    MCFS_CHL_CAL        = 0x0C04,    ///< 12.4  - информация для калибровки каналов МЦФОС;
    // -----------------------------

    MCFS_STATUS_REQ     = 0x0C0A,    ///< 12.10 - запрос состояния МЦФОС;
    MCFS_RESET          = 0x0C12     ///< 12.18 - сброс МЦФОС;

    /// Проверить отсутствие запятой в конце последнего параметра
};

// =========================================
// 	Общий заголовок для сообщений
// =========================================
#pragma pack(push, 1) // Выравнивание по МИНИМАЛЬНОМУ блоку данных в 8бит
/**
 * @struct EthHeader
 * @brief Базовый заголовок для всех Ethernet сообщений протокола
 *
 * @details Структура содержит Mac-заголовок и служебную информацию
 */
struct EthHeader {
    // MAC-заголовок:
    uint8_t     macAddrDst[6];  // MAC-адрес получателя
    uint8_t     macAddrSrc[6];  // MAC-адрес отправителя
    uint16_t    etherType;      // Ethernet-протокол
    // Контроль доставки:
    uint8_t     ctlCommand;     // Команда
    uint16_t    ctlLen;         // Длина пакета
    uint16_t    ctlCnt;         // Счётчик
    uint16_t    ctlNumPack;     // Номер пакета
    uint16_t    ctlNumFrame;    // Номер кадра
    // Заголовок сообщения:
    uint16_t    mesNumber;      // Парный номер, например, номер 11.2 кодируется так: 0x0B02
    uint16_t    mesLength;      // Длина сообщения
    uint8_t     mesReceiver;    // Номер получателя
    uint8_t     mesSender;      // Номер отправителя

    // Методы для сериализации/десериализации
    QByteArray serialize() const {
        return QByteArray(reinterpret_cast<const char*>(this), sizeof(EthHeader));
    }

    static EthHeader deserialize(const QByteArray& data) {
        EthHeader header;
        if(data.size() >= static_cast<int>(sizeof(EthHeader)))
            memcpy(&header, data.constData(), sizeof(EthHeader));
        return header;
    }

    bool isValid() const {
        return true;
    }

protected:

    // Защищенный конструктор для использования в наследниках
    EthHeader(ETH_MessageNumbers msgNumber)
        : macAddrDst(),     // MAC-адрес получателя
          macAddrSrc(),     // MAC-адрес отправителя
          etherType(),      // Ethernet-протокол
          // Контроль доставки:
          ctlCommand(0),    // Команда
          ctlLen(0),        // Длина пакета
          ctlCnt(0),        // Счётчик
          ctlNumPack(0),    // Номер пакета
          ctlNumFrame(0),   // Номер кадра
          // Заголовок сообщения:
          mesNumber(to_underlying(msgNumber)), // Парный номер, например, номер 11.2 кодируется так: 0x0B02
          mesLength(0),     // Длина сообщения
          mesReceiver(0),   // Номер получателя
          mesSender(0){}    // Номер отправителя

    // Для десериализации
    EthHeader() = default;
};
#pragma pack(pop)

// Проверка размера заголовка
static_assert(sizeof(EthHeader) == 29, "Invalid EthHeader size");

// =========================================
// 	Временные метки для сообщений от БС
// =========================================
#pragma pack(push, 1) // Выравнивание по МИНИМАЛЬНОМУ блоку данных в 8бит
/**
 * @struct TimeHeaderFromBS
 * @brief Метки времени для Ethernet сообщений от БС на ВУ
 *
 * @details Структура содержит время приёма и время начала передачи БС
 */
struct TimeHeaderFromBS {
    uint8_t timeReceive[5];     // Время приёма
    uint8_t timeBeginTrans[5];  // Время начала передачи

    // Методы для сериализации/десериализации
    QByteArray serialize() const {
        return QByteArray(reinterpret_cast<const char*>(this), sizeof(TimeHeaderFromBS));
    }

    static TimeHeaderFromBS deserialize(const QByteArray& data) {
        TimeHeaderFromBS header;
        if(data.size() >= static_cast<int>(sizeof(TimeHeaderFromBS)))
            memcpy(&header, data.constData(), sizeof(TimeHeaderFromBS));
        return header;
    }

    bool isValid() const {
        return true;
    }

protected:

    // Защищенный конструктор для использования в наследниках
    TimeHeaderFromBS(uint64_t timeRec, uint64_t timeTrans) {
        for (int i = 0; i < 5; i++) {
            timeReceive[i] = static_cast<uint8_t>(timeRec >> (i * 8)); // Время приёма
        }
        for (int i = 0; i < 5; i++) {
            timeBeginTrans[i] = static_cast<uint8_t>(timeTrans >> (i * 8)); // Время начала передачи
        }
    }

    // Для десериализации
    TimeHeaderFromBS() = default;
};
#pragma pack(pop)

// Проверка размера заголовка
static_assert(sizeof(TimeHeaderFromBS) == 10, "Invalid TimeHeaderFromBS size");

// =========================================
// 	Временные метки для сообщений на БС
// =========================================
#pragma pack(push, 1) // Выравнивание по МИНИМАЛЬНОМУ блоку данных в 8бит
/**
 * @struct TimeHeaderToBS
 * @brief Метки времени для Ethernet сообщений от ВУ на БС
 *
 * @details Структура содержит время начала передачи БС
 */
struct TimeHeaderToBS {
    uint8_t timeBeginTrans[5];  // Время начала передачи

    // Методы для сериализации/десериализации
    QByteArray serialize() const {
        return QByteArray(reinterpret_cast<const char*>(this), sizeof(TimeHeaderToBS));
    }

    static TimeHeaderToBS deserialize(const QByteArray& data) {
        TimeHeaderToBS header;
        if(data.size() >= static_cast<int>(sizeof(TimeHeaderToBS)))
            memcpy(&header, data.constData(), sizeof(TimeHeaderToBS));
        return header;
    }

    bool isValid() const {
        return true;
    }

protected:

    // Защищенный конструктор для использования в наследниках
    TimeHeaderToBS(uint64_t timeTrans) {
        for (int i = 0; i < 5; ++i) {
            timeBeginTrans[i] = static_cast<uint8_t>(timeTrans >> (i * 8)); // Время начала передачи
        }
    }

    // Для десериализации
    TimeHeaderToBS() = default;
};
#pragma pack(pop)

// Проверка размера заголовка
static_assert(sizeof(TimeHeaderToBS) == 5, "Invalid TimeHeaderToBS size");

#endif // ETHMESSAGE_H
