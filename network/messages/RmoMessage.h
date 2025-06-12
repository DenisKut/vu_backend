#pragma once

#include <QByteArray>
#include <QDataStream>
#include <memory>
#include <unordered_map>
#include <functional>
#include "libs/json/json.hpp"

// Добавьте в общий заголовочный файл, если используете C++11
#if __cplusplus < 201402L
namespace std {
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
}
#endif

class RmoMsgType {
public:
    enum Value : int {
        TARGET_PPU = 1,
        ENABLE_PPU = 2,
        RESET_PPU = 3,
		STOP_PPU = 4,
		SET_MODE_PPU = 5,
		PPU_MSG = 6
    };

    // Конструкторы
    constexpr RmoMsgType(Value val) : value_(val) {}
    RmoMsgType() = default;

    // Неявное приведение к int
    constexpr operator int() const noexcept {
        return value_;
    }

    // Операторы сравнения для удобства
    constexpr bool operator==(RmoMsgType other) const noexcept { return value_ == other.value_; }
    constexpr bool operator!=(RmoMsgType other) const noexcept { return value_ != other.value_; }

private:
    Value value_;
};
/*
m_bin[typeId] = &T::fromByteArray;
m_json[typeId] = &T::fromJson;
m_baseConstructorCreator[typeId] = &T::fromBaseConstructor;
*/
struct RmoBaseMessage {
    virtual ~RmoBaseMessage() = default;

    virtual QByteArray toByteArray() const = 0;
    virtual nlohmann::json toJson() const = 0;



    virtual int type() const = 0; // Вернуть номер/тип сообщения

    //-----------------------------------------------------
    //для отладки
    virtual void info() const = 0;

};

/**
 * @struct RmoMessage
 * @brief Сообщение для обмена с RMO (бинарный и WebSocket клиент)
 */
struct RmoMessage_TargetPPU : RmoBaseMessage {
    int32_t typeField = static_cast<int32_t>(RmoMsgType::TARGET_PPU);
    int numPPU = AppConfig::ModulePPU_num_RLMN_1;
    double azPPU = 0.0;
    double anPPU = 0.0;
    double azC0 = 0.0, azC1 = 0.0, azC2 = 0.0;
    double anC0 = 0.0, anC1 = 0.0, anC2 = 0.0;
    double azTarget = 0.0, anTarget = 0.0;
    double azTargetVel = 0.0, anTargetVel = 0.0;
    uint8_t isRemote = 0;
    uint32_t counter = 0;

    int type() const override { return typeField; }

    static int typeStatic() { return static_cast<int>(RmoMsgType::TARGET_PPU); }

    // --- Бинарная сериализация ---
    QByteArray toByteArray() const override {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(Utils::ByteOrder);
        stream << typeField
               << numPPU
               << azPPU << anPPU
               << azC0 << azC1 << azC2
               << anC0 << anC1 << anC2
               << azTarget << anTarget
               << azTargetVel << anTargetVel
               << isRemote << counter;
        return data;
    }

    static QSharedPointer<RmoBaseMessage> fromByteArray(const QByteArray& data) {
        auto msg = QSharedPointer<RmoMessage_TargetPPU>::create();
        QDataStream stream(data);
        stream.setByteOrder(Utils::ByteOrder);
        stream >> msg->typeField
            >> msg->numPPU
            >> msg->azPPU >> msg->anPPU
            >> msg->azC0 >> msg->azC1 >> msg->azC2
            >> msg->anC0 >> msg->anC1 >> msg->anC2
            >> msg->azTarget >> msg->anTarget
            >> msg->azTargetVel >> msg->anTargetVel
            >> msg->isRemote >> msg->counter;
        return msg;
    }

/*
    static QSharedPointer<RmoBaseMessage> fromByteArray(const QByteArray& data) {
        auto msg = QSharedPointer<RmoMessage_TargetPPU>::create();
        QDataStream stream(data);
        stream.setByteOrder(Utils::ByteOrder);
        stream >> msg->typeField
            >> msg->numPPU
            >> msg->azTarget >> msg->anTarget
            >> msg->azTargetVel >> msg->anTargetVel;
        return msg;
    }
    */
    // --- JSON сериализация ---
    nlohmann::json toJson() const override {
        return {
            {"numPPU", numPPU},
            {"azPPU", azPPU}, {"anPPU", anPPU},
            {"azC0", azC0}, {"azC1", azC1}, {"azC2", azC2},
            {"anC0", anC0}, {"anC1", anC1}, {"anC2", anC2},
            {"azTarget", azTarget}, {"anTarget", anTarget},
            {"azTargetVel", azTargetVel}, {"anTargetVel", anTargetVel},
            {"isRemote", isRemote}, {"counter", counter}
        };
    }

    static QSharedPointer<RmoBaseMessage> fromJson(const nlohmann::json& j) {
        auto msg = QSharedPointer<RmoMessage_TargetPPU>::create();
        msg->numPPU = j.value("numPPU",
                              AppConfig::ModulePPU_num_RLMN_1);
        msg->azPPU = j.value("azPPU", 0.0);
        msg->anPPU = j.value("anPPU", 0.0);
        msg->azC0 = j.value("azC0", 0.0);
        msg->azC1 = j.value("azC1", 0.0);
        msg->azC2 = j.value("azC2", 0.0);
        msg->anC0 = j.value("anC0", 0.0);
        msg->anC1 = j.value("anC1", 0.0);
        msg->anC2 = j.value("anC2", 0.0);
        msg->azTarget = j.value("azTarget", 0.0);
        msg->anTarget = j.value("anTarget", 0.0);
        msg->azTargetVel = j.value("azTargetVel", 0.0);
        msg->anTargetVel = j.value("anTargetVel", 0.0);
        msg->isRemote = j.value("isRemote", 0);
        msg->counter = j.value("counter", 0);
        return msg;
    }

    // Конструктор по умолчанию
    RmoMessage_TargetPPU()
        : RmoMessage_TargetPPU(AppConfig::ModulePPU_num_RLMN_1) {}

    RmoMessage_TargetPPU(AppConfig::ModulePPU_num numberPPU)
    {
        numPPU = numberPPU;
        azPPU = AppConfig::instance().getModulePPU_az(numberPPU);
        anPPU = AppConfig::instance().getModulePPU_an(numberPPU);
        azTarget = AppConfig::instance().getModulePPU_azTarget(numberPPU);
        anTarget = AppConfig::instance().getModulePPU_anTarget(numberPPU);
        azTargetVel = AppConfig::instance().getModulePPU_azTargetVel(numberPPU);
        anTargetVel = AppConfig::instance().getModulePPU_anTargetVel(numberPPU);
        isRemote = static_cast<uint8_t>(AppConfig::instance().getModulePPU_isRemote(numberPPU));
    }

    static QSharedPointer<RmoMessage_TargetPPU> fromBaseConstructor()
    {
        return QSharedPointer<RmoMessage_TargetPPU>::create();
    }
    //----------------------------------------------------------------------

    //метод для отладки
    void info() const override
    {
        qInfo() << "RmoMessage_TargetPPU # " << numPPU;
        qInfo() << "azPPU =" << azPPU;
        qInfo() << "anPPU =" << anPPU;
        qInfo() << "azC0 =" << azC0;
        qInfo() << "azC1 =" << azC1;
        qInfo() << "azC2 =" << azC2;
        qInfo() << "anC0 =" << anC0;
        qInfo() << "anC1 =" << anC1;
        qInfo() << "anC2 =" << anC2;
        qInfo() << "azTarget =" << azTarget;
        qInfo() << "anTarget =" << anTarget;
        qInfo() << "azTargetVel =" << azTargetVel;
        qInfo() << "anTargetVel =" << anTargetVel;
    }

    void set_azC(double azC0, double azC1, double azC2)
    {
        this->azC0 = azC0;
        this->azC1 = azC1;
        this->azC2 = azC2;
    }

    void set_anC(double anC0, double anC1, double anC2)
    {
        this->anC0 = anC0;
        this->anC1 = anC1;
        this->anC2 = anC2;
    }
};

/**
 * @struct RmoMessage_EnablePPU
 * @brief Управление состоянием включения ППУ
 */
struct RmoMessage_EnablePPU : RmoBaseMessage {
    int32_t typeField = static_cast<int32_t>(RmoMsgType::ENABLE_PPU);
    int numPPU = AppConfig::ModulePPU_num_RLMN_1;
    // Состояние ВКЛ/ВЫКЛ
    int state = 0;

    int type() const override { return typeField; }

    static int typeStatic() { return static_cast<int>(RmoMsgType::ENABLE_PPU); }

    // --- Бинарная сериализация ---
    QByteArray toByteArray() const override {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(Utils::ByteOrder);
        stream << typeField
               << numPPU
               << state;
        return data;
    }

    static QSharedPointer<RmoBaseMessage> fromByteArray(const QByteArray& data) {
        auto msg = QSharedPointer<RmoMessage_EnablePPU>::create();
        QDataStream stream(data);
        stream.setByteOrder(Utils::ByteOrder);
        stream >> msg->typeField
            >> msg->numPPU
            >> msg->state;
        return msg;
    }

    nlohmann::json toJson() const override {
        return {
            {"numPPU", numPPU},
            {"state", state},
        };
    }

    static QSharedPointer<RmoBaseMessage> fromJson(const nlohmann::json& j) {
        auto msg = QSharedPointer<RmoMessage_EnablePPU>::create();
        msg->numPPU = j.value("numPPU",
                              AppConfig::ModulePPU_num_RLMN_1);
        msg->state = j.value("state", 0);
        return msg;
    }

    RmoMessage_EnablePPU()
        : RmoMessage_EnablePPU(AppConfig::ModulePPU_num_RLMN_1) {}

    RmoMessage_EnablePPU(AppConfig::ModulePPU_num numberPPU)
    {
        numPPU = numberPPU;
        state = AppConfig::ModulePPU_state_TurnedOff;
    }

    static QSharedPointer<RmoMessage_EnablePPU> fromBaseConstructor()
    {
        return QSharedPointer<RmoMessage_EnablePPU>::create();
    }

    void info() const override
    {
        qInfo() << "RmoMessage_EnablePPU # " << numPPU;
        qInfo() << "state =" << state;
    }
};

/**
 * @struct RmoMessage_ResetPPU
 * @brief Сообщение перезагрузки ППУ
 */
struct RmoMessage_ResetPPU : RmoBaseMessage {
    int32_t typeField = static_cast<int32_t>(RmoMsgType::RESET_PPU);
    int numPPU = AppConfig::ModulePPU_num_RLMN_1;
    int resetDst = AppConfig::ModulePPU_resetDst_PPU;

    int type() const override { return typeField; }
    static int typeStatic() { return static_cast<int>(RmoMsgType::RESET_PPU); }

    QByteArray toByteArray() const override {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(Utils::ByteOrder);
        stream << typeField
               << numPPU
               << resetDst;
        return data;
    }

    static QSharedPointer<RmoBaseMessage> fromByteArray(const QByteArray& data) {
        auto msg = QSharedPointer<RmoMessage_ResetPPU>::create();
        QDataStream stream(data);
        stream.setByteOrder(Utils::ByteOrder);
        stream >> msg->typeField
            >> msg->numPPU
            >> msg->resetDst;
        return msg;
    }

    nlohmann::json toJson() const override {
        return {
                {"numPPU", numPPU},
                {"resetDst", resetDst},
                };
    }

    static QSharedPointer<RmoBaseMessage> fromJson(const nlohmann::json& j) {
        auto msg = QSharedPointer<RmoMessage_ResetPPU>::create();
        msg->numPPU = j.value("numPPU", AppConfig::ModulePPU_num_RLMN_1);
        msg->resetDst = j.value("resetDst", AppConfig::ModulePPU_resetDst_PPU);

        return msg;
    }

    RmoMessage_ResetPPU() {
        RmoMessage_ResetPPU(AppConfig::ModulePPU_num_RLMN_1,
                            AppConfig::ModulePPU_resetDst_PPU);
    }

    RmoMessage_ResetPPU(
        AppConfig::ModulePPU_num numberPPU,
        AppConfig::ModulePPU_resetDst dst
    ) {
        numPPU = numberPPU;
        resetDst = dst;
    }

    static QSharedPointer<RmoMessage_ResetPPU> fromBaseConstructor()
    {
        return QSharedPointer<RmoMessage_ResetPPU>::create();
    }

    void info() const override
    {
        qInfo() << "RmoMessage_ResetPPU # " << numPPU;
        qInfo() << "resetDst" << resetDst;
    }
};

/**
 * @struct RmoMessage_StopPPU
 * @brief Сообщение остановки ППУ
 */
struct RmoMessage_StopPPU : RmoBaseMessage {
    int32_t typeField = static_cast<int32_t>(RmoMsgType::STOP_PPU);
    int numPPU = AppConfig::ModulePPU_num_RLMN_1;

    int type() const override { return typeField; }
    static int typeStatic() { return static_cast<int>(RmoMsgType::STOP_PPU); }

    QByteArray toByteArray() const override {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(Utils::ByteOrder);
        stream << typeField
               << numPPU;
        return data;
    }

    static QSharedPointer<RmoBaseMessage> fromByteArray(const QByteArray& data) {
        auto msg = QSharedPointer<RmoMessage_StopPPU>::create();
        QDataStream stream(data);
        stream.setByteOrder(Utils::ByteOrder);
        stream >> msg->typeField
            >> msg->numPPU;
        return msg;
    }

    nlohmann::json toJson() const override {
        return{
                {"numPPU", numPPU},
              };
    }

    static QSharedPointer<RmoBaseMessage> fromJson(const nlohmann::json& j) {
        auto msg = QSharedPointer<RmoMessage_StopPPU>::create();
        msg->numPPU = j.value("numPPU",
                              AppConfig::ModulePPU_num_RLMN_1);
        return msg;
    }

    RmoMessage_StopPPU()
        : RmoMessage_StopPPU(AppConfig::ModulePPU_num_RLMN_1) {}

    RmoMessage_StopPPU(AppConfig::ModulePPU_num numberPPU)
    {
        numPPU = numberPPU;
    }

    static QSharedPointer<RmoMessage_StopPPU> fromBaseConstructor()
    {
        return QSharedPointer<RmoMessage_StopPPU>::create();
    }

    void info() const override
    {
        qInfo() << "RmoMessage_StopPPU # " << numPPU;
    }
};

/**
 * @struct RmoMessage_SetModePPU
 * @brief Сообщение выставления режима движения ППУ
 */
struct RmoMessage_SetModePPU : RmoBaseMessage {
    int32_t typeField = static_cast<int32_t>(RmoMsgType::SET_MODE_PPU);
    int numPPU = AppConfig::ModulePPU_num_RLMN_1;
    int modePPU = AppConfig::ModulePPU_modePPU_speed;

    int type() const override { return typeField; }
    static int typeStatic() { return static_cast<int>(RmoMsgType::SET_MODE_PPU); }

    QByteArray toByteArray() const override {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream << typeField
               << numPPU
               << modePPU;
        return data;
    }

    static QSharedPointer<RmoBaseMessage> fromByteArray(const QByteArray& data) {
        auto msg = QSharedPointer<RmoMessage_SetModePPU>::create();
        QDataStream stream(data);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream >> msg->typeField
            >> msg->numPPU
            >> msg->modePPU;
        return msg;
    }

    nlohmann::json toJson() const override {
        return {
                {"numPPU", numPPU},
                {"modePPU", modePPU}
                };
    }

    static QSharedPointer<RmoBaseMessage> fromJson(const nlohmann::json& j) {
        auto msg = QSharedPointer<RmoMessage_SetModePPU>::create();
        msg->numPPU = j.value("numPPU",
                              AppConfig::ModulePPU_num_RLMN_1);
        msg->modePPU = j.value("modePPU",
                              AppConfig::ModulePPU_modePPU_speed);
        return msg;
    }

    RmoMessage_SetModePPU()
        : RmoMessage_SetModePPU(AppConfig::ModulePPU_num_RLMN_1,
                                AppConfig::ModulePPU_modePPU_speed) {}

    RmoMessage_SetModePPU(
        AppConfig::ModulePPU_num numberPPU,
        AppConfig::ModulePPU_modePPU mode
    ) {
        numPPU = numberPPU;
        modePPU = mode;
    }

    static QSharedPointer<RmoMessage_SetModePPU> fromBaseConstructor()
    {
        return QSharedPointer<RmoMessage_SetModePPU>::create();
    }

    void info() const override
    {
        qInfo() << "RmoMessage_SetModePPU # " << numPPU;
        qInfo() << "modePPU = " << modePPU;
    }
};

/**
 * @struct RmoMessage_MessagePPU
 * @brief Циклически отправляемые сообщения для доведения привода
 */
struct RmoMessage_MessagePPU : RmoBaseMessage {
    int32_t typeField = static_cast<int32_t>(RmoMsgType::PPU_MSG);
    int numPPU = AppConfig::ModulePPU_num_RLMN_1;
    QString direction = "up";

    int type() const override { return typeField; }
    static int typeStatic() { return static_cast<int>(RmoMsgType::PPU_MSG); }

    QByteArray toByteArray() const override {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream << typeField << numPPU;
        QByteArray dirBytes = direction.toUtf8();
        stream.writeRawData(dirBytes.constData(), qMin(dirBytes.size(), 16));
        if (dirBytes.size() < 16) {
            QByteArray padding(16 - dirBytes.size(), '\0');
            stream.writeRawData(padding.constData(), padding.size());
        }
        return data;
    }

    static QSharedPointer<RmoBaseMessage> fromByteArray(const QByteArray& data) {
        auto msg = QSharedPointer<RmoMessage_MessagePPU>::create();
        QDataStream stream(data);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream >> msg->typeField >> msg->numPPU;
        QByteArray dirBytes(16, 0);
        stream.readRawData(dirBytes.data(), 16);
        int realLen = strnlen(dirBytes.constData(), 16);
        msg->direction = QString::fromUtf8(dirBytes.constData(), realLen);
        return msg;
    }

    nlohmann::json toJson() const override {
        return {
            {"numPPU", numPPU},
            {"direction", direction.toStdString()}
        };
    }

    static QSharedPointer<RmoBaseMessage> fromJson(const nlohmann::json& j) {
        auto msg = QSharedPointer<RmoMessage_MessagePPU>::create();
        msg->numPPU = j.value("numPPU", AppConfig::ModulePPU_num_RLMN_1);
        msg->direction = QString::fromStdString(j.value("direction", "up"));
        return msg;
    }

    RmoMessage_MessagePPU()
        : RmoMessage_MessagePPU(AppConfig::ModulePPU_num_RLMN_1, "up") {}

    RmoMessage_MessagePPU(AppConfig::ModulePPU_num numberPPU, QString dir)
    {
        numPPU = numberPPU;
        direction = dir;
    }

    static QSharedPointer<RmoMessage_MessagePPU> fromBaseConstructor() {
        return QSharedPointer<RmoMessage_MessagePPU>::create();
    }

    void info() const override {
        qDebug() << "RmoMessage_MessagePPU # " << numPPU;
        qDebug() << "direction = " << direction;
    }
};

/**
 * @class RmoMessageFactory
 * @brief Фабрика сообщений для реализации реализации
 */
class RmoMessageFactory {
public:
    using ByteArrayCreatorFunc = std::function<QSharedPointer<RmoBaseMessage>(const QByteArray&)>;
    using JsonCreatorFunc = std::function<QSharedPointer<RmoBaseMessage>(const nlohmann::json&)>;
    using BaseConstructorCreatorFunc = std::function<QSharedPointer<RmoBaseMessage>()>;

    static RmoMessageFactory& instance() {
        static RmoMessageFactory inst;
        return inst;
    }


    template<typename T, typename... Args>
    QSharedPointer<T> createWithArgs(Args&&... args) {
        return QSharedPointer<T>::create(std::forward<Args>(args)...);
    }

   //---------------------------------------------------------------------------------------
    template<typename T>
    void registerType() {
        int typeId = T::typeStatic(); // Получаем тип из экземпляра класса
        m_bin[typeId] = &T::fromByteArray;
        m_json[typeId] = &T::fromJson;
        m_baseConstructorCreator[typeId] = &T::fromBaseConstructor;
    }

    // Создание из QByteArray с автоматическим определением типа
    QSharedPointer<RmoBaseMessage> fromByteArray(const QByteArray& data) {
        if (data.size() < (int)sizeof(int32_t)) return nullptr;

        // Читаем тип из первых байтов данных
        int32_t type;
        QDataStream stream(data);
        stream.setByteOrder(Utils::ByteOrder);
        stream >> type;

        if (m_bin.count(type)) {
            return m_bin[type](data);
        }
        return nullptr;
    }

    // Создание из JSON с автоматическим определением типа
    //если добавить в формирование json указание типа, то будет работать
    /*
    QSharedPointer<RmoBaseMessage> fromJson(const nlohmann::json& j) {
        if (!j.contains("type")) return nullptr;

        int type = j["type"];
        if (m_json.count(type)) {
            return m_json[type](j);
        }
        return nullptr;
    }
    */

    //БАЗА ОСНОВА---------------------------------------------------------------------------

    QSharedPointer<RmoBaseMessage> fromByteArray(int type, const QByteArray& data) {
        if (m_bin.count(type)) return m_bin[type](data);
        return nullptr;
    }

    QSharedPointer<RmoBaseMessage> fromJson(int type, const nlohmann::json& j) {
        if (m_json.count(type)) return m_json[type](j);
        return nullptr;
    }

    QSharedPointer<RmoBaseMessage> fromBaseConstructor(int type) {
        if (m_baseConstructorCreator.count(type))
        {
            return m_baseConstructorCreator[type]();
        }
        return nullptr;
    }

    template<typename Derived>
    QSharedPointer<Derived> getDerived(QSharedPointer<RmoBaseMessage> basePtr) {
        return basePtr.dynamicCast<Derived>();
    }

    //-----------------------------------------------------------------------------


private:
    std::unordered_map<int, ByteArrayCreatorFunc> m_bin;
    std::unordered_map<int, JsonCreatorFunc> m_json;
    std::unordered_map<int, BaseConstructorCreatorFunc> m_baseConstructorCreator;

    RmoMessageFactory() {
        registerType<RmoMessage_TargetPPU>();
        registerType<RmoMessage_EnablePPU>();
        registerType<RmoMessage_ResetPPU>();
        registerType<RmoMessage_StopPPU>();
        registerType<RmoMessage_SetModePPU>();
        registerType<RmoMessage_MessagePPU>();
    }
};
