#ifndef TCPMESSAGE_H
#define TCPMESSAGE_H

#pragma once
#include <cstdint>
#include <cstring>
#include <iostream>
#include <QByteArray>

#include "utils/AppConfig.h"

#include "libs/json/json.hpp"

using json = nlohmann::json;

struct TcpMessage {
    quint64 timestamp;
    QByteArray payload;
};

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
// Общий заголовок для сообщений
// =========================================
#pragma pack(push, 1)
struct TcpHeader
{
	// Word - 1
	// * Идентификатор отправителя данных *
    uint32_t sender: 24;
	// * Номер модуля в комплексе *
	uint8_t idxModule;
	// Word - 2
	// * Версия протокола мажор *
	uint8_t vMajor;
	// * Версия протокола минор *
	uint8_t vMinor;
	// * Сквозной порядковый номер пакета *
	uint16_t idxPack;
	// Word - 3
	// * Размер блока данных *
	uint32_t sizeData: 31;
	// * Флаг ПО АСКУ *
	uint8_t isAsku: 1;
	// Word - 4
	// * № типа передаваемого пакета *
	// !!! из PBU_MessageNumbers !!!
	uint16_t typePack;
	// * Контрольная сумма заголовка *
	uint16_t checkSum;

    TcpHeader()
        :sender(AppConfig::instance().PBU_PROTO_HEAD_SENDER()),
        idxModule(AppConfig::instance().PBU_PROTO_HEAD_IDX_MODULE()),
        vMajor(AppConfig::instance().PBU_PROTO_MAJOR_VERSION()),
        vMinor(AppConfig::instance().PBU_PROTO_MINOR_VERSION()),
        idxPack(AppConfig::instance().PBU_PROTO_HEAD_IDX_PACK()),
        isAsku(AppConfig::instance().PBU_PROTO_HEAD_IS_ASKU())
            {}
	
	~TcpHeader() {}
};
#pragma pack(pop)

// * От ВУ на ПБУ [ОБРАБОТКА ИНФ] *
// *********************************
// REQ_REGISTRATION (0x01)
// *********************************

#endif // TCPMESSAGE_H
