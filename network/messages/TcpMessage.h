#ifndef TCPMESSAGE_H
#define TCPMESSAGE_H

#pragma once
#include <cstdint>
#include <cstring>
#include <iostream>
#include <QByteArray>
#include <type_traits>

#include "utils/AppConfig.h"
#include "libs/json/json.hpp"

using json = nlohmann::json;

// =========================================
// Вспомогательные функции и утилиты
// =========================================

// Проверка контрольной суммы
static inline uint16_t calculateChecksum(const uint8_t* data, size_t size) {
    uint16_t checksum = 0;
    for (size_t i = 0; i < size; ++i) {
        checksum += data[i];
    }
    return checksum;
}

// Преобразование enum class в базовый тип
template <typename E>
constexpr auto to_underlying(E e) -> typename std::underlying_type<E>::type {
    return static_cast<typename std::underlying_type<E>::type>(e);
}

// =========================================
// Номера сообщений по протоколу связи с ПБУ
// =========================================
enum class PBU_MessageNumbers: uint16_t {
    // -----------------------------------------
    // Сообщения от ВУ на ПБУ [ОБРАБОТКА ИНФОРМАЦИИ]
    // -----------------------------------------
    REQ_REGISTRATION    = 0x01,      // Запрос регистрации модуля в системе;
    REQ_TIME            = 0x03,      // Запрос времени (пинг);
    GEO_MODULE          = 0x05,      // Геопозиция модуля;
    MARK_TRAICE         = 0x09,      // Отметка трассы (Отмечает наше ВУ)
    CURRENT_STATE_RLS   = 0x1021,    // Текущие состояния РЛС-Н (1-й и 2-й
									 // отдельно);
    MARK_TARGET         = 0x1022,    // Отметка цели;
    MODULE_CAPABILITIES = 0x1023,    // Возможности модуля;
    FORBIDDEN_SECTORS   = 0x1024,    // Слепые зоны;

    // -----------------------------------------
    // Сообщения от ПБУ на ВУ [ОБРАБОТКА ИНФОРМАЦИИ]
    // -----------------------------------------
    RES_REGISTRATION        = 0x02, // Подтверждение регистрации модуля в
									// системе;
    TIMESTAMP               = 0x04, // Отметка времени;
    GEO_COMPLEX             = 0x06, // Геопозиция комплекса (присылается один 
									// раз;
                                    // при подключении);
    MARK_TRAICE_PBU         = 0x07, // Отметка трассы (ПБУ присылает все текущие
                                    // трассы – по умолчанию отключено);
    ORIENTATION_SC_COMPLEX  = 0x08, // Ориентация СК комплекса (вместе с
									// геопозицией => один раз);
    TARGET_DESIGNATION      = 0x641,// Команда «Целеуказание»;

    // -----------------------------------------
    // Сообщения от ВУ на ПБУ [КОНТРОЛЯ И УПРАВЛЕНИЯ]
    // -----------------------------------------
    MODULE_SCHEME                       = 0x20, // Схема модуля;
    STATUS_SCHEME_ELEMENTS              = 0x21, // Статус элементов схемы;
    VALUES_KP_STATUS_SCHEME_ELEMENTS    = 0x22, // Значения и статус КП
												// элементов схемы;
    RECEIPT_CONTROL_COMMAND             = 0x23, // Квитанция на получение
												// команды управления;
    STATUS_MODULE                       = 0x24, // Статус Модуля;
    TEXT_FOR_MESSAGE_LOG                = 0x25, // Текстовые сообщения в журнал
												// сообщений;
    TEXT_FOR_OPERATOR                   = 0x26, // Текстовые сообщения
												// оператору;
    RECEIPT_CONTROL_COMMAND_EXTENDED    = 0x27, // Квитанция на получение
												// команды управления 
												// РАСШИРЕННАЯ;
    VALUES_CUSTOMIZEBLE_PARAMETERS      = 0x28, // Значения настраиваемых
												// параметров;

    // -----------------------------------------
    // Сообщения от ПБУ на ВУ [КОНТРОЛЯ И УПРАВЛЕНИЯ]
    // -----------------------------------------
    COMMAND_SWITCH_ON               = 0x40, // Команда «Включить»;
    COMMAND_SWITCH_OFF              = 0x41, // Команда «Выключить»;
    COMMAND_RESTART                 = 0x42, // Команда «Перезагрузить»;
    COMMAND_RESET_ALARM             = 0x43, // Команда «Сброс аварии»;
    COMMAND_SET_CORRECTIONS         = 0x44, // Команда «Установить поправки»;
    TEXT_FOR_OPERATOR_PBU           = 0x45, // Текстовые сообщения оператору;
    REQ_MODULE_SCHEME               = 0x46, // Запрос схемы модуля;
    COMMAND_SET_LANGUAGE            = 0x47, // Команда «Установить язык
											// элементов схемы и текстовых 
											// сообщений»;
    MESSAGE_SATELLITE_NAV           = 0x48, // Сообщение «Включено подавление/
											// искажение спутниковой навигации»;
    COMMAND_SET_TIME                = 0x49, // Команда «Установить время»;
    COMMAND_SET_COORDINATES_MODULE  = 0x4A, // Команда «Установить координаты 
											// модуля»;
    COMMAND_SET_SETTINGS_VALUES     = 0x4B, // Команда Установить значения
											// настраиваемых параметров»;
};

// =========================================
// 	Общий заголовок для сообщений
// =========================================
#pragma pack(push, 1) // Выравнивание по МИНИМАЛЬНОМУ блоку данных в 8бит
struct TcpHeader {
	// Word - 1
    uint32_t sender: 24;    // * Идентификатор отправителя данных *
    uint32_t idxModule: 8;  // * Номер модуля в комплексе *
	// Word - 2
    uint8_t vMajor;         // * Версия протокола мажор *
    uint8_t vMinor;         // * Версия протокола минор *
    uint16_t idxPack;       // * Сквозной порядковый номер пакета *
	// Word - 3
    uint32_t sizeData: 31;  // * Размер блока данных *
    uint32_t isAsku: 1;     // * Флаг ПО АСКУ *
	// Word - 4
    uint16_t typePack;      // * № типа передаваемого пакета *
                            // !!! из PBU_MessageNumbers !!!
    uint16_t checkSum;      // * Контрольная сумма заголовка *

    // Методы для сериализации/десериализации
    QByteArray serialize() const {
        return QByteArray(reinterpret_cast<const char*>(this), sizeof(TcpHeader));
    }

    static TcpHeader deserialize(const QByteArray& data) {
        TcpHeader header;
        if(data.size() >= static_cast<int>(sizeof(TcpHeader)))
            memcpy(&header, data.constData(), sizeof(TcpHeader));
        return header;
    }

    bool isValid() const {
        const size_t headerSize = sizeof(TcpHeader) - sizeof(checkSum);
        return checkSum == calculateChecksum(
                   reinterpret_cast<const uint8_t*>(this), headerSize);
    }

protected:

    // Защищенный конструктор для использования в наследниках
    TcpHeader(PBU_MessageNumbers msgType, uint32_t dataSize)
        : sender(AppConfig::instance().PBU_PROTO_HEAD_SENDER()),
        idxModule(AppConfig::instance().PBU_PROTO_HEAD_IDX_MODULE()),
        vMajor(AppConfig::instance().PBU_PROTO_MAJOR_VERSION()),
        vMinor(AppConfig::instance().PBU_PROTO_MINOR_VERSION()),
        idxPack(AppConfig::instance().PBU_PROTO_HEAD_IDX_PACK()),
        sizeData(dataSize),
        isAsku(AppConfig::instance().PBU_PROTO_HEAD_IS_ASKU()),
        typePack(to_underlying(msgType)),
        checkSum(0) // Инициализируем нулем перед расчетом
    {
        // Рассчет контрольной суммы только для заголовка
        const auto headerSize = sizeof(TcpHeader) - sizeof(checkSum);
        checkSum = calculateChecksum(
            reinterpret_cast<const uint8_t*>(this), headerSize
            );
    }

    // Для десериализации
    TcpHeader() = default;
};
#pragma pack(pop)

// Проверка размера заголовка
static_assert(sizeof(TcpHeader) == 16, "Invalid TcpHeader size");

// =========================================
// Шаблон для создания конкретных сообщений
// =========================================
template <typename TDerived, PBU_MessageNumbers MsgType>
struct TcpMessage : TcpHeader {
    static constexpr PBU_MessageNumbers MESSAGE_TYPE = MsgType;

    TcpMessage() : TcpHeader(MsgType, sizeof(TDerived) - sizeof(TcpHeader)) {}

    // Сериализация полного сообщения
    QByteArray serializeFull() const {
        QByteArray data(sizeof(TDerived), 0);
        memcpy(data.data(), this, sizeof(TDerived));
        return data;
    }

    // Десериализация
    static TDerived deserialize(const QByteArray& data) {
        TDerived message;
        if (data.size() >= static_cast<int>(sizeof(TDerived))) {
            memcpy(&message, data.constData(), sizeof(TDerived));
        }
        return message;
    }

    // // Валидация сообщения
    // bool isValid() const {
    //     // Проверка контрольной суммы
    //     const auto headerSize = sizeof(TcpHeader) - size_t(checkSum);
    //     const auto expectedChecksum = calculateChecksum(
    //         reinterpret_cast<const uint8_t*>(this), headerSize
    //         );
    //     return checkSum == expectedChecksum;
    // }
};

// =========================================
// Конкретные реализации сообщений
// =========================================

// * От ВУ на ПБУ [ОБРАБОТКА ИНФ] *
// *********************************
// REQ_REGISTRATION (0x01)
// *********************************
#pragma pack(push, 1)
struct TcpMessage_0x01 : TcpMessage<TcpMessage_0x01, PBU_MessageNumbers::REQ_REGISTRATION> {
    // Word - 1
    uint32_t idManuf: 8;        // * Идентификатор производителя *
    uint32_t serialNum: 24;     // * Серийный номер изделия *
    // Word - 2
    uint8_t versHardMaj;        // * Версия модификации изделия мажор *
    uint8_t versHardMin;        // * Версия модификации изделия минор *
    uint8_t versProgMaj: 7;     // * Версия ПО нашего ВУ (мажор)*
    uint8_t isInfo: 1;          // * Флаг ПО обработки информации (состояние
                                //   для обработчиков протокола) *
    uint8_t versProgMin: 7;     // * Версия ПО нашего ВУ (минор) *
    uint8_t isControl: 1;       // * Флаг ПО контроля и управления *

    TcpMessage_0x01() {
        // Инициализация полей из конфига
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

static_assert(sizeof(TcpMessage_0x01) == 24, "Invalid TcpMessage_0x01 size");

// =========================================
// Фабрика для работы с сообщениями
// =========================================
class MessageFactory {
public:
    // Создание сообщения по типу
    template<typename T>
    static typename std::enable_if<std::is_base_of<TcpHeader, T>::value, T>::type create() {
        return T();
    }

    // Парсинг входящих данных
    static std::unique_ptr<TcpHeader> parse(const QByteArray& data) {
        if (data.size() < static_cast<int>(sizeof(TcpHeader))) {
            return nullptr;
        }

        // 1. Десериализуем базовый заголовок
        TcpHeader header = TcpHeader::deserialize(data);
        if (!header.isValid()) {
            return nullptr;
        }

        // 2. Определяем тип сообщения
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
