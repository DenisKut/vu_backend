#pragma once
#include <cstdint>
#include <QString>
#include <QHash>

// Макросы для приведения типов
#define UINT32(X) static_cast<uint32_t>(X)
#define UINT16(X) static_cast<uint16_t>(X)
#define UINT8(X)  static_cast<uint8_t>(X)
#define STR(X)    QString::fromUtf8(X)

// Структура с константами из .env
struct AppConstants {
    static uint8_t PBU_PROTOCOL_MAJOR_VERSION;
    static uint8_t PBU_PROTOCOL_MINOR_VERSION;

    // Инициализация (вызывается после загрузки .env)
    static void init(const QHash<QString, QString>& env) {
        PBU_PROTOCOL_MAJOR_VERSION = UINT8(env.value("PBU_PROTOCOL_MAJOR_VERSION", "2").toUShort());
        PBU_PROTOCOL_MINOR_VERSION = UINT8(env.value("PBU_PROTOCOL_MINOR_VERSION", "0").toUShort());
    }
};
