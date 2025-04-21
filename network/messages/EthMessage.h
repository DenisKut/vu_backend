#ifndef ETHMESSAGE_H
#define ETHMESSAGE_H

#pragma once
#include <cstdint>
#include <cstring>
#include <iostream>
#include <QByteArray>
#include <QDebug>
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
        // Дополнение кол-ва данных до 60, если кол-во данных меньше:
        if (data.size() < 60) data.append(60 - data.size(), 0);
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
 * @struct EthMessage_PPU_STOP
 * @brief Команда остановки ППУ (11.4)
 */
#pragma pack(push, 1)
struct EthMessage_PPU_STOP : EthMessage<EthMessage_PPU_STOP, ETH_MessageNumbers::PPU_STOP> {
    // Метки времени:
    uint8_t     timeBeginTrans[5];  ///< Время начала передачи
    // Данные:
    // Поле данных отсутствует

    EthMessage_PPU_STOP() {
        uint64_t time = TimeProvider::instance().getBsPeriodTime();
        memcpy(timeBeginTrans, &time, 5); // Копирует 5 младших байтов
//        timeBeginTrans[0] = time & 0xFF;
//        timeBeginTrans[1] = (time >> 8) & 0xFF;
//        timeBeginTrans[2] = (time >> 16) & 0xFF;
//        timeBeginTrans[3] = (time >> 24) & 0xFF;
//        timeBeginTrans[4] = (time >> 32) & 0xFF;
    }
};
#pragma pack(pop)

static_assert(sizeof(EthMessage_PPU_STOP) == 5 + 29, "Invalid size [EthMessage_PPU_STOP]");

/**
 * @struct EthMessage_PPU_ENABLE
 * @brief Команда включения ППУ (11.6)
 */
#pragma pack(push, 1)
struct EthMessage_PPU_ENABLE : EthMessage<EthMessage_PPU_ENABLE, ETH_MessageNumbers::PPU_ENABLE> {
    // Метки времени:
    uint8_t     timeBeginTrans[5];  ///< Время начала передачи
    // Данные:
    uint8_t     state;              ///< Состояние ППУ (0 – выключить ППУ, 1 – включить ППУ)

//    EthMessage_PPU_ENABLE(uint8_t _state) {
    EthMessage_PPU_ENABLE() {
        uint64_t time = TimeProvider::instance().getBsPeriodTime();
        memcpy(timeBeginTrans, &time, 5); // Копирует 5 младших байтов
//        state = _state > 0 ? 1 : 0;
//        timeBeginTrans[0] = time & 0xFF;
//        timeBeginTrans[1] = (time >> 8) & 0xFF;
//        timeBeginTrans[2] = (time >> 16) & 0xFF;
//        timeBeginTrans[3] = (time >> 24) & 0xFF;
//        timeBeginTrans[4] = (time >> 32) & 0xFF;
    }
};
#pragma pack(pop)

static_assert(sizeof(EthMessage_PPU_ENABLE) == 6 + 29, "Invalid size [EthMessage_PPU_ENABLE]");

/**
 * @struct EthMessage_PPU_MOVE
 * @brief Целеуказание для ППУ (11.8)
 */
#pragma pack(push, 1)
struct EthMessage_PPU_MOVE : EthMessage<EthMessage_PPU_MOVE, ETH_MessageNumbers::PPU_MOVE> {
    // Метки времени:
    uint8_t     timeBeginTrans[5];  ///< Время начала передачи
    // Данные:
    uint8_t     azC0[3];            ///< Коэффициент C0 полинома для движения по азимуту, град
    uint16_t    azC1;               ///< Коэффициент C1 полинома для движения по азимуту, град/с
    uint16_t    azC2;               ///< Коэффициент C2 полинома для движения по азимуту, град/с^2
    uint8_t     anC0[3];            ///< Коэффициент C0 полинома для движения по углу места, град
    uint16_t    anC1;               ///< Коэффициент C1 полинома для движения по углу места, град/с
    uint16_t    anC2;               ///< Коэффициент C2 полинома для движения по углу места, град/с^2
    uint16_t    azSector;           ///< Размер области по азимуту, град
    uint16_t    anSector;           ///< Размер области по углу места, град

    EthMessage_PPU_MOVE() {
        uint64_t time = TimeProvider::instance().getBsPeriodTime();
        memcpy(timeBeginTrans, &time, 5); // Копирует 5 младших байтов
//        timeBeginTrans[0] = time & 0xFF;
//        timeBeginTrans[1] = (time >> 8) & 0xFF;
//        timeBeginTrans[2] = (time >> 16) & 0xFF;
//        timeBeginTrans[3] = (time >> 24) & 0xFF;
//        timeBeginTrans[4] = (time >> 32) & 0xFF;

        /// @warning !!! Для заполнения требует наличия заведённых структур для
        /// повороток
    }
};
#pragma pack(pop)

static_assert(sizeof(EthMessage_PPU_MOVE) == 23 + 29, "Invalid size [EthMessage_PPU_MOVE]");

/**
 * @struct MCFS_Object
 * @brief Обнаруженный объект (для сообщения EthMessage_MCFS_RESULT - 15.1)
 */
#pragma pack(push, 1)
struct MCFS_Object {
    uint8_t     doplerCh;   ///< Номер доплеровского канала объекта
    uint16_t    delay;      ///< Код времени задержки максимума сигнала
                            ///< объекта относительно начала участка обработки

    int16_t     re1;        ///< Действительная составляющая амплитуды сигнала в 1-ом канале
    int16_t     im1;        ///< Мнимая составляющая амплитуды сигнала в 1-ом  канале

    int16_t     re2;        ///< Действительная составляющая амплитуды сигнала во 2-ом канале
    int16_t     im2;        ///< Мнимая составляющая амплитуды сигнала во 2-ом  канале

    int16_t     re3;        ///< Действительная составляющая амплитуды сигнала в 3-ем канале
    int16_t     im3;        ///< Мнимая составляющая амплитуды сигнала в 3-ем  канале

    int16_t     re4;        ///< Действительная составляющая амплитуды сигнала в 4-ом канале
    int16_t     im4;        ///< Мнимая составляющая амплитуды сигнала в 4-ом  канале

    int16_t     re5;        ///< Действительная составляющая амплитуды сигнала в 5-ом канале
    int16_t     im5;        ///< Мнимая составляющая амплитуды сигнала в 5-ом  канале
};
#pragma pack(pop)

static_assert(sizeof(MCFS_Object) == 23, "Invalid size [MCFS_Object]");

/**
 * @struct EthMessage_MCFS_RESULT
 * @brief Информация по обнаруженным объектам (15.1)
 */
#pragma pack(push, 1)
struct EthMessage_MCFS_RESULT : EthMessage<EthMessage_MCFS_RESULT, ETH_MessageNumbers::MCFS_RESULT> {
    // Метки времени:
    uint8_t     timeReceive[5];     ///< Время приёма
    uint8_t     timeBeginTrans[5];  ///< Время начала передачи
    // Данные:
    uint8_t     timeStartGen[5];    ///<  0-4  Код времени начала генерации первого импульса в пачке
    uint32_t    taskNum;            ///<  5-8  Номер задания
    uint8_t     signalType;         ///<    9  Код вида сигнала
    uint8_t     impulseNum : 4;     ///<   10 [0-3] Код количества импульсов сигнала СОТС в пачке
    uint8_t     reserv1 : 2;        ///<   10 [4-5] Резерв
    uint8_t     isFreqMod : 1;      ///<   10  [6]  Признак изменения частоты ЛЧМ сигналов СОТС
    uint8_t     reserv2 : 1;        ///<   10  [7]  Резерв
    uint16_t    period;             ///< 11-12 Код периода повторения импульсов в пачке
    uint8_t     reserv3[12];        ///< 13-24 Резерв
    uint16_t    duration;           ///< 25-26 Код длительности одного интервала обработки сигнала
                                    ///<       (без учета длительности импульса)
    uint8_t     reserv4;            ///<   27  Резерв
    uint8_t     amplify;            ///<   28  Код коэффициента передачи приемных каналов УПЧ,
                                    ///<       (внешний модуль усиления)
    uint8_t     koefFalse;          ///<   29  Коэффициент, определяющий вероятность ложной тревоги
    uint16_t    treshold;           ///< 30-31 Уровень постоянного порога
    uint8_t     regType : 1;        ///<   32 [0] Признак вида регистрации
    uint8_t     regCh1 : 1;         ///<   32 [1] Признак регистрации сигнатур в 1-ом канале
    uint8_t     regCh2 : 1;         ///<   32 [2] Признак регистрации сигнатур во 2-ом канале
    uint8_t     regCh3 : 1;         ///<   32 [3] Признак регистрации сигнатур в 3-ем канале
    uint8_t     regCh4 : 1;         ///<   32 [4] Признак регистрации сигнатур в 4-ом канале
    uint8_t     regCh5 : 1;         ///<   32 [5] Признак регистрации сигнатур в 5-ом канале
                                    ///<          УЦП (в приемном канале КИБ)
    uint8_t     reserv5 : 2;        ///<   32 [6-7] Резерв
    uint8_t     reserv6[3];         ///<   33-35 Резерв
    uint8_t     numObjects;         ///<   36 Количество обнаруженных объектов на интервале обработки

    /// Массив обнаруженных объектов
    QVector<MCFS_Object> vecObjects;

    /// Метод для добавления обнаруженных объектов
    void addObject(const MCFS_Object& obj) {
        vecObjects.append(obj);
    }

    /// Проверка количества обнаруженных объектов
    bool isValid() const {
        return (vecObjects.size() <= 60);
    }
};
#pragma pack(pop)

static_assert(sizeof(EthMessage_MCFS_RESULT) == 47 + sizeof(QVector<MCFS_Object>) + 29, "Invalid size [EthMessage_MCFS_RESULT]");

/**
 * @struct MCFS_Sample
 * @brief Выборка сигнала (для сообщения EthMessage_MCFS_RESULT - 15.1)
 */
#pragma pack(push, 1)
struct MCFS_Sample {
    int16_t     re;        ///< Действительная составляющая комплексной амплитуды сигнала
    int16_t     im;        ///< Мнимая составляющая комплексной амплитуды сигнала

    // Метод для десериализации из массива char* data
    static MCFS_Sample fromBytes(const char* data) {
        MCFS_Sample s;
        memcpy(&s, data, sizeof(s));
//        // Конвертирование многоразрядных слов
//        s.re = qFromLittleEndian(s.re);
//        s.im = qFromLittleEndian(s.im);
        return s;
    }
};
#pragma pack(pop)

static_assert(sizeof(MCFS_Sample) == 4, "Invalid size [MCFS_Sample]");

/**
 * @struct EthMessage_MCFS_REG
 * @brief Информация регистрации (15.3)
 */
#pragma pack(push, 1)
struct EthMessage_MCFS_REG : EthMessage<EthMessage_MCFS_REG, ETH_MessageNumbers::MCFS_REG> {
    // Метки времени:
    uint8_t     timeReceive[5];     ///< Время приёма
    uint8_t     timeBeginTrans[5];  ///< Время начала передачи
    // Данные:
    uint8_t     timeStartGen[5];    ///<  0-4  Код времени начала первого интервала обработки
                                    ///<       в пачке при регистрации принимаемого сигнала
                                    ///<       или
                                    ///<       Код времени начала генерации первого импульса
                                    ///<       в пачке при регистрации излучаемого сигнала
    uint32_t    taskNum;            ///<  5-8  Номер задания
    uint8_t     lastMes : 1;        ///<   9 [0] Признак завершающего сообщения
    uint8_t     regType : 1;        ///<   9 [1] Признак вида регистрации
    uint8_t     regCh6 : 1;         ///<   9 [2] Признак регистрации сигнатур в 6-ом канале
    uint8_t     regCh5 : 1;         ///<   9 [3] Признак регистрации сигнатур в 5-ом канале
                                    ///<          УЦП (в приемном канале КИБ)
    uint8_t     regCh4 : 1;         ///<   9 [4] Признак регистрации сигнатур в 4-ом канале
    uint8_t     regCh3 : 1;         ///<   9 [5] Признак регистрации сигнатур в 3-ем канале
    uint8_t     regCh2 : 1;         ///<   9 [6] Признак регистрации сигнатур во 2-ом канале
    uint8_t     regCh1 : 1;         ///<   9 [7] Признак регистрации сигнатур в 1-ом канале
    uint8_t     chIntNum;           ///<  10   Номер доплеровского канала сигнала СОТС,
                                    ///<       прошедшего пороговую обработку при regType = 1,
                                    ///<       или Номер интервала обработки при regType = 0
    uint32_t    signalLen;          ///< 11-12 Количество зарегистрированных выборок
                                    ///<       в доплеровском канале (на интервале обработки)
    uint8_t     reserv[2];          ///< 13-14 Резерв
    uint32_t    mesNum;             ///< 15-18 Порядковый номер сообщения в массиве
                                    ///<       передаваемых сообщений 15.3

    /// Массив выборок сигнала
    QVector<MCFS_Sample> vecSamples;

    /// Метод для десериализации в структуру
    bool deserialize(const QByteArray& data) {
        /// Размер фиксированной части структуры:
        const size_t FIXED_SIZE = sizeof(EthMessage_MCFS_REG) -
                                  sizeof(QVector<MCFS_Sample>);

        if(static_cast<size_t>(data.size()) < FIXED_SIZE) {
            qDebug() << "ОШИБКА: EthMessage_MCFS_REG::deserialize - "
                        "кол-во данных меньше минимально необходимого!";
            return false;
        }

        // Копирование части структуры
        // фиксированного размера
        memcpy(this, data.constData(), FIXED_SIZE);

//        // Конвертирование многоразрядных слов
//        p4 = qFromLittleEndian(p4);
//        p6 = qFromLittleEndian(p6);
//        p8 = qFromLittleEndian(p8);

        // Копирование части структуры
        // переменного размера в вектор
        //-------------------------------------------------------
        const char* vecData = data.constData() + FIXED_SIZE;
        const size_t vecSizeBytes = data.size() - FIXED_SIZE;
        const size_t vecItemSize = sizeof(MCFS_Sample);

        if(vecSizeBytes % vecItemSize != 0) {
            qDebug() << "ОШИБКА: EthMessage_MCFS_REG::deserialize - "
                        "переменная часть данных неправильного размера!";
            return false;
        }

        const size_t itemCount = vecSizeBytes / vecItemSize;
        vecSamples.resize(itemCount);

        for(size_t i = 0; i < itemCount; ++i) {
            vecSamples[i] = MCFS_Sample::fromBytes(vecData + i * vecItemSize);
        }
        //-------------------------------------------------------

        // Копирование в структуру завершено успешно
        return true;
    }
};
#pragma pack(pop)

static_assert(sizeof(EthMessage_MCFS_REG) == 31 + sizeof(QVector<MCFS_Sample>) + 29, "Invalid size [EthMessage_MCFS_REG]");

/**
 * @struct EthMessage_MCFS_STATUS
 * @brief Состояние устройства (12.11)
 */
#pragma pack(push, 1)
struct EthMessage_MCFS_STATUS : EthMessage<EthMessage_MCFS_STATUS, ETH_MessageNumbers::MCFS_STATUS> {
    // Метки времени:
    uint8_t     timeReceive[5];     ///< Время приёма
    uint8_t     timeBeginTrans[5];  ///< Время начала передачи
    // Данные:
    uint8_t     state;              ///< Код состояния устройства
    uint8_t     error;              ///< Код ошибки при приеме
    uint8_t     reserv : 6;         ///< Резерв
    uint8_t     isprPPM : 1;        ///< Признак исправности ППМ
    uint8_t     isTabu : 1;         ///< Признак наличия ЗС
    uint8_t     error12;            ///< Код ошибки при обработке
                                    ///< сообщений 12.0, 12.1, 12.2
                                    ///< (поле значимо только для этих сообщений)
    uint8_t     timeCommandRec[5];  ///< Код времени устройства на момент
                                    ///< получения сообщения от ВУ
};
#pragma pack(pop)

static_assert(sizeof(EthMessage_MCFS_STATUS) == 19 + 29, "Invalid size [EthMessage_MCFS_STATUS]");

// MCFS_SGNL_GEN       = 0x010C,    ///< 12.1  - генерация сигнала МЦФОС;
// MCFS_SGNL_REC       = 0x020C,    ///< 12.2  - приём сигнала МЦФОС;
// MCFS_SGNL_GEN_REC   = 0x030C,    ///< 12.3  - генерация и приём сигнала МЦФОС;

///**
// * @struct EthMessage_MCFS_SGNL_GEN_REC
// * @brief Генерация и приём сигнала МЦФОС (12.3)
// */
//#pragma pack(push, 1)
//struct EthMessage_MCFS_SGNL_GEN_REC : EthMessage<EthMessage_MCFS_SGNL_GEN_REC, ETH_MessageNumbers::MCFS_SGNL_GEN_REC> {
//    // Метки времени:
//    uint8_t     timeBeginTrans[5];  ///< Время начала передачи
//    // Данные:
//    uint8_t     timeStartGen[5];    ///<  0-4  Код времени начала генерации первого импульса в пачке
//    uint32_t    taskNum;            ///<  5-8  Номер задания
//    uint8_t     signalType;         ///<    9  Код вида сигнала
//    uint8_t     impulseNum : 4;     ///<   10 [0-3] Код количества импульсов сигнала СОТС в пачке
//    uint8_t     reserv1 : 2;        ///<   10 [4-5] Резерв
//    uint8_t     isFreqMod : 1;      ///<   10  [6]  Признак изменения частоты ЛЧМ сигналов СОТС
//    uint8_t     reserv2 : 1;        ///<   10  [7]  Резерв
//    uint16_t    period;             ///< 11-12 Код периода повторения импульсов в пачке
//    uint8_t     reserv3[12];        ///< 13-24 Резерв
//    uint16_t    duration;           ///< 25-26 Код длительности одного интервала обработки сигнала
//                                    ///<       (без учета длительности импульса)
//    uint8_t     reserv4;            ///<   27  Резерв
//    uint8_t     amplify;            ///<   28  Код коэффициента передачи приемных каналов УПЧ,
//                                    ///<       (внешний модуль усиления)
//    uint8_t     koefFalse;          ///<   29  Коэффициент, определяющий вероятность ложной тревоги
//    uint16_t    treshold;           ///< 30-31 Уровень постоянного порога
//    uint8_t     regType : 1;        ///<   32 [0] Признак вида регистрации
//    uint8_t     regCh1 : 1;         ///<   32 [1] Признак регистрации сигнатур в 1-ом канале
//    uint8_t     regCh2 : 1;         ///<   32 [2] Признак регистрации сигнатур во 2-ом канале
//    uint8_t     regCh3 : 1;         ///<   32 [3] Признак регистрации сигнатур в 3-ем канале
//    uint8_t     regCh4 : 1;         ///<   32 [4] Признак регистрации сигнатур в 4-ом канале
//    uint8_t     regCh5 : 1;         ///<   32 [5] Признак регистрации сигнатур в 5-ом канале
//                                    ///<          УЦП (в приемном канале КИБ)
//    uint8_t     reserv5 : 2;        ///<   32 [6-7] Резерв
//    uint8_t     reserv6[3];         ///<   33-35 Резерв

//    EthMessage_MCFS_SGNL_GEN_REC() {
//        uint64_t time = TimeProvider::instance().getBsPeriodTime();
//        memcpy(timeBeginTrans, &time, 5); // Копирует 5 младших байтов
////        timeBeginTrans[0] = time & 0xFF;
////        timeBeginTrans[1] = (time >> 8) & 0xFF;
////        timeBeginTrans[2] = (time >> 16) & 0xFF;
////        timeBeginTrans[3] = (time >> 24) & 0xFF;
////        timeBeginTrans[4] = (time >> 32) & 0xFF;

//        /// @warning !!! Для заполнения требует наличия заведённых структур для МЦФОС
//    }
//};
//#pragma pack(pop)

//static_assert(sizeof(EthMessage_MCFS_SGNL_GEN_REC) == 23 + 29, "Invalid size [EthMessage_MCFS_SGNL_GEN_REC]");


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
