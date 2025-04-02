#pragma once
#include <cstdint>
#include <QString>
#include <QHash>

// Inline функции для приведения типов
template<typename T>
static inline uint32_t to_uint32(T x) { return static_cast<uint32_t>(x); }     // * Приведение к uint_32 *

template<typename T>
static inline uint16_t to_uint16(T x) { return static_cast<uint16_t>(x); }     // * Приведение к uint_16 *

template<typename T>
static inline uint8_t  to_uint8(T x)  { return static_cast<uint8_t>(x); }      // * Приведение к uint_8 *

// * Получение строки QString (например, для QSocket)*
static inline QString to_qstring(const char* x) { return QString::fromUtf8(x); }
static inline QString to_qstring(const std::string& x) { return QString::fromStdString(x); }
static inline QString to_qstring(const QString& x) { return x; }

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

    // Геттеры параметров из .env
    std::string TCP_HOST_IP() const {
        return this->_tcpHostIP;
    }
    int TCP_HOST_PORT() const {
        return this->_tcpHostPort;
    }
    int UDP_PORT() const {
        return this->_udpPort;
    }
    std::string ETH_INTERFACE_NAME() const {
        return this->_ethInterfaceName;
    }
    int MAC_SOCKET_BUF_SIZE() const {
        return this->_macSocketBuffSize;
    }


    uint8_t PBU_PROTO_MAJOR_VERSION() const {
        return this->_pbuProtoMajorVersion;
    }
    uint8_t PBU_PROTO_MINOR_VERSION() const {
        return this->_pbuProtoMinorVersion;
    }
    uint32_t PBU_PROTO_HEAD_SENDER() const {
        return this->_pbuProtoHeadSender;
    }

    uint8_t PBU_PROTO_0X01_ID_MANUF() const {
        return this->_pbuProto0x01IdManuf;
    }
    uint32_t PBU_PROTO_0X01_SERIAL_NUM() const {
        return this->_pbuProto0x01SerialNum;
    }
    uint8_t PBU_PROTO_0X01_VERS_HARD_MAJ() const {
        return this->_pbuProto0x01VersHardMaj;
    }
    uint8_t PBU_PROTO_0X01_VERS_HARD_MIN() const {
        return this->_pbuProto0x01VersHardMin;
    }
    uint8_t PBU_PROTO_0X01_VERS_PROG_MAJ() const {
        return this->_pbuProto0x01VersProgMaj;
    }
    uint8_t PBU_PROTO_0X01_VERS_PROG_MIN() const {
        return this->_pbuProto0x01VersProgMin;
    }

    // *******************************************
    //	Переменные протокола взаимодействия с ПБУ
    // *******************************************
    // ----------------------------
    // 	Для tcpHeader
    // ----------------------------
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

    // ----------------------------
    // 	REQ_REGISTRATION (0x01)
    // ----------------------------
    uint8_t PBU_PROTO_0X01_IS_INFO() const {
        return this->_pbuProto0x01IsInfo;
    }
    void SET_PBU_PROTO_0X01_IS_INFO(bool flag) {
        flag ?
            this->_pbuProto0x01IsInfo = 1 :
            this->_pbuProto0x01IsInfo = 0;
    }
    uint8_t PBU_PROTO_0X01_IS_CONTROL() const {
        return this->_pbuProto0x01IsControl;
    }
    void SET_PBU_PROTO_0X01_IS_CONTROL(bool flag) {
        flag ?
            this->_pbuProto0x01IsControl = 1 :
            this->_pbuProto0x01IsControl = 0;
    }


    // Инициализация из .env
    void initFromEnv(const QHash<QString, QString>& env) {
        this->_tcpHostIP = env.value("TCP_HOST_IP", "127.0.0.1").toStdString();
        this->_tcpHostPort = env.value("TCP_HOST_PORT", "12345").toInt();
        this->_udpPort = env.value("UDP_PORT", "54321").toInt();
        this->_ethInterfaceName = env.value("ETH_INTERFACE_NAME", "eth0").toStdString();
        this->_macSocketBuffSize = env.value("MAC_SOCKET_BUF_SIZE", "2048").toInt();

        this->_pbuProtoMajorVersion = to_uint8(env.value("PBU_PROTO_MAJOR_VERSION", "2").toUShort());
        this->_pbuProtoMinorVersion = to_uint8(env.value("PBU_PROTO_MINOR_VERSION", "0").toUShort());
        this->_pbuProtoHeadSender = to_uint32(env.value("PBU_PROTO_HEAD_ID_SENDER", "0x524C4E").toUInt());

        this->_pbuProto0x01IdManuf = to_uint8(env.value("PBU_PROTO_0X01_ID_MANUF", "0x24").toUShort());
        this->_pbuProto0x01SerialNum = to_uint32(env.value("PBU_PROTO_0X01_SERIAL_NUM", "1").toUInt());
        this->_pbuProto0x01VersHardMaj = to_uint8(env.value("PBU_PROTO_0X01_VERS_HARD_MAJ", "1").toUShort());
        this->_pbuProto0x01VersHardMin = to_uint8(env.value("PBU_PROTO_0X01_VERS_HARD_MIN", "0").toUShort());
        this->_pbuProto0x01VersProgMaj = to_uint8(env.value("PBU_PROTO_0X01_VERS_PROG_MAJ", "1").toUShort());
        this->_pbuProto0x01VersProgMin = to_uint8(env.value("PBU_PROTO_0X01_VERS_PROG_MIN", "0").toUShort());

    }

    //  Инициализация других параметров
    void initBase() {
        // Инициализация значений для заголовков сообщений ПБУ
        this->_pbuProtoHeadIdxModule = PBU_PROTO_HEAD_IDX_MODULE_FIRST;
        this->_pbuProtoHeadIdxPack = 0;
        this->_pbuProtoHeadIsAsku = 1;

        // Инициализация значений по телу сообщений ПБУ
        // ---- 0x01 -----
        this->_pbuProto0x01IsInfo = 0;
        this->_pbuProto0x01IsControl = 0;

    }

    void init(const QHash<QString, QString>& env) {
        initFromEnv(env);
        initBase();
    }

private:
    // Приватный конструктор
    AppConfig() = default;

    // !!! Параметры для .env !!!
    std::string _tcpHostIP;
    int _tcpHostPort;
    int _udpPort;
    std::string _ethInterfaceName;
    int _macSocketBuffSize;

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

    uint8_t _pbuProto0x01IdManuf;           // * ID отправителя при запросе регистрации *
    uint32_t _pbuProto0x01SerialNum: 24;    // * Серийный номер изделия *
    uint8_t _pbuProto0x01VersHardMaj;       // * Версия модификации изделия мажор *
    uint8_t _pbuProto0x01VersHardMin;       // * Версия модификации изделия минор *
    uint8_t _pbuProto0x01VersProgMaj: 7;    // * Версия ПО нашего ВУ (мажор) *
    uint8_t _pbuProto0x01IsInfo: 1;         // * Флаг ПО обработки информации (состояние
                                            //   для обработчиков протокола) *
                                            // ! > ВАЖНО: Данный флаг ВСЕГДА меняется
                                            // ! > при полученном сообщении в ЧАСТИ
                                            // !ОБРАБОТКИ
    uint8_t _pbuProto0x01VersProgMin: 7;    // * Версия ПО нашего ВУ (мажор) *
    uint8_t _pbuProto0x01IsControl: 1;      // * Флаг ПО контроля и управления *
                                            // ! > ВАЖНО: Данный флаг ВСЕГДА меняется
                                            // ! > при полученном сообщении в ЧАСТИ КОНТРОЛЯ и УПРАВЛЕНИЯ
    // uint8_t _pbuProto0x01;

};
