#pragma once
#include <cstdint>
#include <QString>
#include <QHash>

// Макросы для приведения типов
#define UINT32(X) static_cast<uint32_t>(X)	// * Приведение к uint_32 *
#define UINT16(X) static_cast<uint16_t>(X)	// * Приведение к uint_16 *
#define UINT8(X)  static_cast<uint8_t>(X)	// * Приведение к uint_8 *
#define STR(X)    QString::fromUtf8(X)     	// * Получение строки формата UTF-8 *

class AppConfig {
public:
    // Удаляем копирование и присваивание
    AppConfig(const AppConfig&) = delete;
    AppConfig& operator=(const AppConfig&) = delete;

    // Получение экземпляра синглтона
    static AppConfig& instance() {
        static AppConfig config;
        return config;
    }

    // =============================================
    //  Константы и параметры протоколов
    // =============================================

    // Базовые константы
    static constexpr uint8_t PBU_PROTO_HEAD_IDX_MODULE_FIRST = 0xFF;	// * До подтверждения регистрации
																		//   (В ответном пакете нам
																		//   приcваивается idx Module) *
    static constexpr uint16_t PBU_PROTO_0X01_ID_MANUF = 0x24;			// * ID отправителя при запросе
																		// 	 регистрации *

    // Геттеры параметров из .env
    uint8_t PBU_PROTO_MAJOR_VERSION() const {
        return _pbuProtoMajorVersion;
    }
    uint8_t PBU_PROTO_MINOR_VERSION() const {
        return _pbuProtoMinorVersion;
    }
    uint32_t PBU_PROTO_HEAD_SENDER() const {
        return _pbuProtoHeadSender;
    }

    // *******************************************
    //	Переменные протокола взаимодействия с ПБУ
    // *******************************************
    // ---------------------
    // 	Для tcpHeader
    // ---------------------
    uint8_t PBU_PROTO_HEAD_IDX_MODULE() const {
        return _pbuProtoHeadIdxModule;
    }
    void SET_PBU_PROTO_HEAD_IDX_MODULE(uint8_t data) {
        this->_pbuProtoHeadIdxModule = data;
    }
    uint16_t PBU_PROTO_HEAD_IDX_PACK() const {
        return this->_pbuProtoHeadIdxPack;
    }
    void INCREMENT_PBU_PROTO_HEAD_IDX_PACK() {
        this->_pbuProtoHeadIdxPack++;
    }
    uint8_t PBU_PROTO_HEAD_IS_ASKU() const {
        return this->_pbuProtoHeadIsAsku;
    }
    void SET_PROTO_HEAD_IS_ASKU(bool flag) {
        flag ?
            this->_pbuProtoHeadIsAsku = 1 :
            this->_pbuProtoHeadIsAsku = 0;
    }

    // Инициализация из .env
    void initFromEnv(const QHash<QString, QString>& env) {
        this->_pbuProtoMajorVersion = UINT8(env.value("PBU_PROTO_MAJOR_VERSION", "2").toUShort());
        this->_pbuProtoMinorVersion = UINT8(env.value("PBU_PROTO_MINOR_VERSION", "0").toUShort());
        this->_pbuProtoHeadSender = UINT32(env.value("PBU_PROTO_HEAD_ID_SENDER", "0x524C4E").toUShort());
    }

    void initBase() {
        // Инициализация других параметров
        this->_pbuProtoHeadIdxModule = PBU_PROTO_HEAD_IDX_MODULE_FIRST;
        this->_pbuProtoHeadIdxPack = 0;
        this->_pbuProtoHeadIsAsku = 1;
    }

    void init(const QHash<QString, QString>& env) {
        initFromEnv(env);
        initBase();
    }

private:
    // Приватный конструктор
    AppConfig() = default;

    // !!! Параметры для .env !!!
    uint8_t _pbuProtoMajorVersion;          // * Версия протокола (minor & major)*
    uint8_t _pbuProtoMinorVersion;
    uint32_t _pbuProtoHeadSender: 24;       // * Идентификатор отправителя данных *

    // !!! Параметры для Протоколов !!!
    // *** Протокол подключения с ПБУ ***
    uint8_t _pbuProtoHeadIdxModule;         // * Номер модуля в комплексе *
    uint16_t _pbuProtoHeadIdxPack;          // * Сквозной порядковый номер пакета *
    uint8_t _pbuProtoHeadIsAsku: 1;         // * Флаг ПО АСКУ *
                                            // ! > Если в модуле используется отдельное ПО для
                                            // ! > контроля и управления и сообщение отправляется
                                            // ! > ПО АСКУ, то в данное поле выставляется «1», в
                                            // ! > остальных случаях - «0»
};
