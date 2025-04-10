#ifndef ETHMESSAGE_H
#define ETHMESSAGE_H

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

// Преобразование enum class в базовый тип
template <typename E>
constexpr auto to_underlying_eth(E e) -> typename std::underlying_type<E>::type {
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
    // НОМЕРА ИНВЕРТИРОВАНЫ (номер 2.5 записывается как 5.2 = 0x0502)

    // -----------------------------------------
    // Сообщения от БС на ВУ [ВРЕМЯ]
    // -----------------------------------------
    BS_SYNC             = 0x0502,    ///< 2.5   - сообщение синхронизации БС;

    // -----------------------------------------
    // Сообщения от БС на ВУ [ПРИВОД]
    // -----------------------------------------
    PPU_POSITION        = 0x010B,    ///< 11.1  - информация ориентации ППУ;

    // -----------------------------------------
    // Сообщения от ВУ на БС [ПРИВОД]
    // -----------------------------------------
    PPU_STOP            = 0x040B,    ///< 11.4  – команда остановки ППУ;
    PPU_ENABLE          = 0x060B,    ///< 11.6  - команда включения ППУ;
    PPU_MOVE            = 0x080B,    ///< 11.8  - целеуказание для ППУ;

    // -----------------------------------------
    // Сообщения от БС на ВУ [МЦФОС]
    // -----------------------------------------
    MCFS_RESULT         = 0x010F,    ///< 15.1  - информация по обнаруженным объектам;
    MCFS_REG            = 0x030F,    ///< 15.3  - информация регистрации;
    MCFS_STATUS         = 0x0B0C,    ///< 12.11 - состояние устройства;

    // -----------------------------------------
    // Сообщения от ВУ на БС [МЦФОС]
    // -----------------------------------------
    MCFS_SGNL_GEN       = 0x010C,    ///< 12.1  - генерация сигнала МЦФОС;
    MCFS_SGNL_REC       = 0x020C,    ///< 12.2  - приём сигнала МЦФОС;
    MCFS_SGNL_GEN_REC   = 0x030C,    ///< 12.3  - генерация и приём сигнала МЦФОС;

    // -------НЕ ИСПОЛЬЗОВАТЬ-------
    MCFS_CHL_CAL        = 0x040C,    ///< 12.4  - информация для калибровки каналов МЦФОС;
    // -----------------------------

    MCFS_STATUS_REQ     = 0x0A0C,    ///< 12.10 - запрос состояния МЦФОС;
    MCFS_RESET          = 0x120C     ///< 12.18 - сброс МЦФОС;
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
    uint8_t     macAddrDst[6];  ///< MAC-адрес получателя
    uint8_t     macAddrSrc[6];  ///< MAC-адрес отправителя
    uint16_t    etherType;      ///< Ethernet-протокол
    // Контроль доставки:
    uint8_t     ctlCommand;     ///< Команда
    uint16_t    ctlLen;         ///< Длина пакета
    uint16_t    ctlCnt;         ///< Счётчик
    uint16_t    ctlNumPack;     ///< Номер пакета
    uint16_t    ctlNumFrame;    ///< Номер кадра
    // Заголовок сообщения:
    uint16_t    mesNumber;      ///< Парный номер, например, номер 11.2 кодируется так: 0x0B02
    uint16_t    mesLength;      ///< Длина сообщения
    uint8_t     mesReceiver;    ///< Номер получателя
    uint8_t     mesSender;      ///< Номер отправителя

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
    EthHeader(ETH_MessageNumbers msgNumber, uint64_t sizeOfPacket)
        : macAddrDst(),     // MAC-адрес получателя
          macAddrSrc(),     // MAC-адрес отправителя
          etherType(AppConfig::instance().ETH_PROTO_ETHER_TYPE()),      // Ethernet-протокол
          // Контроль доставки:
          ctlCommand(0),    // Команда (всегда 0)
          ctlLen(static_cast<uint16_t>(sizeOfPacket - 14)),        // Длина пакета без учёта Mac-заголовка
          ctlCnt(0),        // Счётчик (всегда 0)
          ctlNumPack(0),    // Номер пакета (всегда 0)
          ctlNumFrame(0),   // Номер кадра (всегда 0)
          mesNumber(to_underlying_eth(msgNumber)),    // Номер сообщения
          mesLength(static_cast<uint16_t>(sizeOfPacket - 23)),     // Длина сообщения, начиная с номера сообщения
          mesReceiver(0),   // Номер получателя (всегда 0)
          mesSender(0)      // Номер отправителя (всегда 0)
    {
        uint64_t _ethProtoMacDst = AppConfig::instance().ETH_PROTO_MAC_DST();
        uint64_t _ethProtoMacSrc = AppConfig::instance().ETH_PROTO_MAC_SRC();
        for (int i = 0; i < 6; i++)
        {
            macAddrDst[i] = static_cast<uint8_t>(_ethProtoMacDst >> (i * 8));
            macAddrSrc[i] = static_cast<uint8_t>(_ethProtoMacSrc >> (i * 8));
        }
    }

    // Для десериализации
    EthHeader() = default;
};
#pragma pack(pop)

// Проверка размера заголовка
static_assert(sizeof(EthHeader) == 29, "Invalid EthHeader size");

// =========================================
// Шаблон для создания конкретных сообщений
// =========================================

/**
 * @struct EthMessage
 * @brief Шаблон базового класса для типизированных сообщений
 * @tparam TDerived Конкретный тип сообщения
 * @tparam MsgType Идентификатор типа сообщения
 */
template <typename TDerived, ETH_MessageNumbers MsgType>
struct EthMessage : EthHeader {
    static constexpr ETH_MessageNumbers MESSAGE_TYPE = MsgType; ///< Тип сообщения

    /// @brief Конструктор инициализирует заголовок
    EthMessage() : EthHeader(MsgType, sizeof(TDerived)) {}

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

/**
 * @struct EthMessage_BS_SYNC
 * @brief Сообщение синхронизации БС (2.5)
 */
#pragma pack(push, 1)
struct EthMessage_BS_SYNC : EthMessage<EthMessage_BS_SYNC, ETH_MessageNumbers::BS_SYNC> {
    // Метки времени:
    uint8_t     timeReceive[5];     ///< Время приёма
    uint8_t     timeBeginTrans[5];  ///< Время начала передачи
    // Данные:
    uint32_t    periodNumber;       ///< Порядковый номер нового периода
    uint8_t     periodTime[5];      ///< Время начала нового периода
    uint8_t     reserv[3];          ///< Резерв
//    uint64_t    periodTime : 40;    ///< Время начала нового периода
//    uint64_t    reserv : 24;        ///< Резерв
    uint32_t    periodWidth;        ///< Значение периода синхронизации
};
#pragma pack(pop)

static_assert(sizeof(EthMessage_BS_SYNC) == 26 + 29, "Invalid size [EthMessage_BS_SYNC]");

/**
 * @struct EthMessage_PPU_POSITION
 * @brief Информация ориентации ППУ (11.1)
 */
#pragma pack(push, 1)
struct EthMessage_PPU_POSITION : EthMessage<EthMessage_PPU_POSITION, ETH_MessageNumbers::PPU_POSITION> {
    // Метки времени:
    uint8_t     timeReceive[5];     ///< Время приёма
    uint8_t     timeBeginTrans[5];  ///< Время начала передачи
    // Данные:
    uint8_t     lastCommand;        ///< Код последней полученной команды
    uint8_t     state;              ///< Состояние
    uint8_t     ispr;               ///< Исправность оборудования
    uint8_t     az[3];              ///< Текущая координата по азимуту, град
    uint8_t     azDelay;            ///< Задержка измерения координаты по азимуту, мс
    uint8_t     an[3];              ///< Текущая координата по углу места, град
    uint8_t     anDelay;            ///< Задержка измерения координаты по углу места, мс
    uint8_t     borders;            ///< Состояние границ
    uint8_t     waitCommands;       ///< Время ожидания команд, мс
    uint8_t     waitTarget;         ///< Время ожидания целеуказаний, мс
};
#pragma pack(pop)

static_assert(sizeof(EthMessage_PPU_POSITION) == 24 + 29, "Invalid size [EthMessage_PPU_POSITION]");


/**
 * @class EthMessageFactory
 * @brief Фабрика для создания и парсинга сообщений Ethernet
 */
class EthMessageFactory {
public:
    /**
     * @brief Создает сообщение указанного типа
     * @tparam T Тип сообщения (должен наследовать от EthHeader)
     * @return Экземпляр сообщения
     */
    template<typename T>
    static typename std::enable_if<std::is_base_of<EthHeader, T>::value, T>::type create() {
        return T();
    }

    /**
     * @brief Парсит входящие данные в сообщение
     * @param data Входные данные
     * @return Указатель на сообщение или nullptr при ошибке
     */
    static std::unique_ptr<EthHeader> parse(const QByteArray& data) {
        if (data.size() < static_cast<int>(sizeof(EthHeader))) {
            return nullptr;
        }

        /// @brief Десериализуем базовый заголовок
        EthHeader header = EthHeader::deserialize(data);
        if (!header.isValid()) {
            return nullptr;
        }

        /// @brief Определяем тип сообщения
        switch (static_cast<ETH_MessageNumbers>(header.mesNumber)) {
        case ETH_MessageNumbers::BS_SYNC: {
            return parseMessage<EthMessage_BS_SYNC>(data);
        }
        case ETH_MessageNumbers::PPU_POSITION: {
            return parseMessage<EthMessage_PPU_POSITION>(data);
        }
        // Обработка других типов...
        default: return nullptr;
        }
    }

private:
    /**
     * @brief Парсит сообщение типа T из данных
     * @private
     * @tparam T Тип сообщения (наследник EthHeader)
     * @param data Входные данные
     * @return Уникальный указатель на сообщение или nullptr при ошибке
     */
    template<typename T>
    static std::unique_ptr<EthHeader> parseMessage(const QByteArray& data) {
        if(data.size() < static_cast<int>(sizeof(T))) return nullptr;

        T msg = T::deserialize(data);
        if(!msg.isValid()) return nullptr;

        return std::unique_ptr<EthHeader>(new T(msg));
    }
};

///////////////////////////////////////////////////////
/// Пример работы с сообщениями
///////////////////////////////////////////////////////
/**
 * @example Пример использования фабрики сообщений
 * @code
 * // Создание сообщения
 * auto msg = EthMessageFactory::create<EthMessage_BS_SYNC>();
 * QByteArray data = msg.serializeFull();
 *
 * // Парсинг
 * auto parsed = EthMessageFactory::parse(data);
 * // Используем reinterpret_cast, потому что parsed.get() - unique_ptr
 * if(auto msgBS_SYNC = reinterpret_cast<EthMessage_BS_SYNC*>(parsed.get())) {
 *    // Работа с сообщением...
 * }
 * @endcode
 */

#endif // ETHMESSAGE_H
