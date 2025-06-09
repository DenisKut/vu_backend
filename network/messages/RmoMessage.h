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
QSharedPointer<T> make_unique(Args&&... args) {
    return QSharedPointer<T>(new T(std::forward<Args>(args)...));
}
}
#endif

enum class RmoMsgType : int {
    TARGET_PPU = 1,
    ENABLE_PPU = 2,
    RESET_PPU = 3,
    STOP_PPU = 4,
    MCFOS_WRITE_DAC_REF = 5,
    MCFOS_START = 6
};

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
    int numPPU = AppConfig::ModulePPU_num::ModulePPU_num_RLMN_1;
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
        stream.setByteOrder(QDataStream::LittleEndian);
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
        stream.setByteOrder(QDataStream::LittleEndian);
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
        stream.setByteOrder(QDataStream::LittleEndian);
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
                              AppConfig::ModulePPU_num::ModulePPU_num_RLMN_1);
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
        : RmoMessage_TargetPPU(AppConfig::ModulePPU_num::ModulePPU_num_RLMN_1) {}

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
    int numPPU = AppConfig::ModulePPU_num::ModulePPU_num_RLMN_1;
    // Состояние ВКЛ/ВЫКЛ
    int state = 0;

    int type() const override { return typeField; }

    static int typeStatic() { return static_cast<int>(RmoMsgType::ENABLE_PPU); }

    // --- Бинарная сериализация ---
    QByteArray toByteArray() const override {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream << typeField
               << numPPU
               << state;
        return data;
    }

    static QSharedPointer<RmoBaseMessage> fromByteArray(const QByteArray& data) {
        auto msg = QSharedPointer<RmoMessage_EnablePPU>::create();
        QDataStream stream(data);
        stream.setByteOrder(QDataStream::LittleEndian);
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
                              AppConfig::ModulePPU_num::ModulePPU_num_RLMN_1);
        msg->state = j.value("state", 0);
        return msg;
    }

    RmoMessage_EnablePPU()
        : RmoMessage_EnablePPU(AppConfig::ModulePPU_num::ModulePPU_num_RLMN_1) {}

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
    int numPPU = AppConfig::ModulePPU_num::ModulePPU_num_RLMN_1;

    int type() const override { return typeField; }
    static int typeStatic() { return static_cast<int>(RmoMsgType::RESET_PPU); }

    QByteArray toByteArray() const override {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream << typeField
               << numPPU;
        return data;
    }

    static QSharedPointer<RmoBaseMessage> fromByteArray(const QByteArray& data) {
        auto msg = QSharedPointer<RmoMessage_ResetPPU>::create();
        QDataStream stream(data);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream >> msg->typeField
            >> msg->numPPU;
        return msg;
    }

    nlohmann::json toJson() const override {
        return {
                {"numPPU", numPPU},
                };
    }

    static QSharedPointer<RmoBaseMessage> fromJson(const nlohmann::json& j) {
        auto msg = QSharedPointer<RmoMessage_ResetPPU>::create();
        msg->numPPU = j.value("numPPU",
                              AppConfig::ModulePPU_num::ModulePPU_num_RLMN_1);
        return msg;
    }

    RmoMessage_ResetPPU()
        : RmoMessage_ResetPPU(AppConfig::ModulePPU_num::ModulePPU_num_RLMN_1) {}

    RmoMessage_ResetPPU(AppConfig::ModulePPU_num numberPPU)
    {
        numPPU = numberPPU;
    }

    static QSharedPointer<RmoMessage_ResetPPU> fromBaseConstructor()
    {
        return QSharedPointer<RmoMessage_ResetPPU>::create();
    }

    void info() const override
    {
        qInfo() << "RmoMessage_ResetPPU # " << numPPU;
    }
};

/**
 * @struct RmoMessage_StopPPU
 * @brief Сообщение остановки ППУ
 */
struct RmoMessage_StopPPU : RmoBaseMessage {
    int32_t typeField = static_cast<int32_t>(RmoMsgType::STOP_PPU);
    int numPPU = AppConfig::ModulePPU_num::ModulePPU_num_RLMN_1;

    int type() const override { return typeField; }
    static int typeStatic() { return static_cast<int>(RmoMsgType::STOP_PPU); }

    QByteArray toByteArray() const override {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream << typeField
               << numPPU;
        return data;
    }

    static QSharedPointer<RmoBaseMessage> fromByteArray(const QByteArray& data) {
        auto msg = QSharedPointer<RmoMessage_StopPPU>::create();
        QDataStream stream(data);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream >> msg->typeField
            >> msg->numPPU;
        return msg;
    }

    nlohmann::json toJson() const override {
        return {
                {"numPPU", numPPU},
                };
    }

    static QSharedPointer<RmoBaseMessage> fromJson(const nlohmann::json& j) {
        auto msg = QSharedPointer<RmoMessage_StopPPU>::create();
        msg->numPPU = j.value("numPPU",
                              AppConfig::ModulePPU_num::ModulePPU_num_RLMN_1);
        return msg;
    }

    RmoMessage_StopPPU()
        : RmoMessage_StopPPU(AppConfig::ModulePPU_num::ModulePPU_num_RLMN_1) {}

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
 * @struct RmoMessage_MCFOS_Write_DAC_REF
 * @brief Сообщение для записи опорных сигналов в MCFOS
 */
struct RmoMessage_MCFOS_Write_DAC_REF : RmoBaseMessage {
    int32_t typeField = static_cast<int32_t>(RmoMsgType::MCFOS_WRITE_DAC_REF);
    float amlitude;
    float f0;
    float df;
    quint32 windowType;

    int type() const override { return typeField; }
    static int typeStatic() { return static_cast<int>(RmoMsgType::MCFOS_WRITE_DAC_REF); }

    QByteArray toByteArray() const override {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream << typeField << amlitude << f0 << df << windowType;
        return data;
    }
    static QSharedPointer<RmoBaseMessage> fromByteArray(const QByteArray& data) {
        auto msg = QSharedPointer<RmoMessage_MCFOS_Write_DAC_REF>::create();
        QDataStream stream(data);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream >> msg->typeField >> msg->amlitude >> msg->f0
            >> msg->df >> msg->windowType;
        return msg;
    }
    nlohmann::json toJson() const override {
        nlohmann::json j;
        j["amlitude"] = amlitude;
        j["f0"] = f0;
        j["df"] = df;
        j["windowType"] = windowType;
        return j;
    }

    static QSharedPointer<RmoBaseMessage> fromJson(const nlohmann::json& j) {
        auto msg = QSharedPointer<RmoMessage_MCFOS_Write_DAC_REF>::create();
        msg->amlitude = j.value("amlitude", 0);
        msg->f0 = j.value("f0", 0);
        msg->df = j.value("df", 0);
        msg->windowType = j.value("windowType", 0);
        return msg;
    }

    void info() const override {
        qInfo() << "RmoMessage_MCFOS_Write_DAC_REF:";
        qInfo() << "amlitude = " << amlitude;
        qInfo() << "f0 = " << f0;
        qInfo() << "df = " << df;
        qInfo() << "windowType = " << windowType;
    }

    static QSharedPointer<RmoMessage_MCFOS_Write_DAC_REF> fromBaseConstructor() {
        return QSharedPointer<RmoMessage_MCFOS_Write_DAC_REF>::create();
    }
};

/**
 * @struct RmoMessage_MCFOS_Start
 * @brief Сообщение для старта MCFOS
 */
struct RmoMessage_MCFOS_Start : RmoBaseMessage {
    int32_t typeField = static_cast<int32_t>(RmoMsgType::MCFOS_START);
    quint32 pulseSize;
    quint32 periodCount;
    quint32 pulseOn;
    quint32 pulseOff;
    quint32 ampOff;
    quint32 procDelay;
    quint32 procDelta;
    quint32 periodSize;
    quint32 windowType;

    int type() const override { return typeField; }
    static int typeStatic() { return static_cast<int>(RmoMsgType::MCFOS_START); }

    QByteArray toByteArray() const override {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream << typeField << pulseSize << periodCount << pulseOn
               << pulseOff
               << ampOff
               << procDelay
               << procDelta
               << periodSize
               << windowType;
        return data;
    }
    static QSharedPointer<RmoBaseMessage> fromByteArray(const QByteArray& data) {
        auto msg = QSharedPointer<RmoMessage_MCFOS_Start>::create();
        QDataStream stream(data);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream >> msg->typeField >> msg->pulseSize >> msg->periodCount
            >> msg->pulseOn >> msg->pulseOff >> msg->ampOff >> msg->procDelay
            >> msg->procDelta >> msg->periodSize >> msg->windowType;
        return msg;
    }
    nlohmann::json toJson() const override {
        nlohmann::json j;
        j["pulseSize"] = pulseSize;
        j["periodCount"] = periodCount;
        j["pulseOn"] = pulseOn;
        j["pulseOff"] = pulseOff;
        j["ampOff"] = ampOff;
        j["procDelay"] = procDelay;
        j["procDelta"] = procDelta;
        j["periodSize"] = periodSize;
        j["windowType"] = windowType;
        return j;
    }
    static QSharedPointer<RmoBaseMessage> fromJson(const nlohmann::json& j) {
        auto msg = QSharedPointer<RmoMessage_MCFOS_Start>::create();
        msg->pulseSize = j.value("pulseSize", 0);
        msg->periodCount = j.value("periodCount", 0);
        msg->pulseOn = j.value("pulseOn", 0);
        msg->pulseOff = j.value("pulseOff", 0);
        msg->ampOff = j.value("ampOff", 0);
        msg->procDelay = j.value("procDelay", 0);
        msg->procDelta = j.value("procDelta", 0);
        msg->periodSize = j.value("periodSize", 0);
        msg->windowType = j.value("windowType", 0);
        return msg;
    }
    static QSharedPointer<RmoMessage_MCFOS_Start> fromBaseConstructor() {
        return QSharedPointer<RmoMessage_MCFOS_Start>::create();
    }
    void info() const override {
        qInfo() << "RmoMessage_MCFOS_Start:";
        qInfo() << "pulseSize = " << pulseSize;
        qInfo() << "periodCount = " << periodCount;
        qInfo() << "pulseOn = " << pulseOn;
        qInfo() << "pulseOff = " << pulseOff;
        qInfo() << "ampOff = " << ampOff;
        qInfo() << "procDelay = " << procDelay;
        qInfo() << "procDelta = " << procDelta;
        qInfo() << "periodSize = " << periodSize;
        qInfo() << "pulseSize = " << pulseSize;
        qInfo() << "windowType = " << windowType;
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

    template<typename T>
    QSharedPointer<T> createFromByteArray(const QByteArray& data) {
        int typeId = T::typeStatic();
        if (m_bin.count(typeId)) {
            auto basePtr = fromByteArray(typeId, data);
            return getDerived<T>(basePtr);
        }
        return nullptr;
    }
    //чтобы не хранить в мапе, но доступ только через параметры шаблона
    template<typename T>
    QSharedPointer<T> createFromByteArray2(const QByteArray& data) {

        return getDerived<T>(T::fromByteArray(data));
    }

    template<typename T>
    QSharedPointer<T> createFromJson(const nlohmann::json& j) {
        int typeId = T::typeStatic();
        if (m_json.count(typeId)) {
            auto basePtr = fromJson(typeId, j);
            return getDerived<T>(basePtr);
        }
        return nullptr;
    }

    template<typename T>
    QSharedPointer<T> createFromJson2(const nlohmann::json& j) {
              return getDerived<T>(T::fromJson(j));
    }

    template<typename T>
    QSharedPointer<T> createFromBaseConstructor() {
        int typeId = T::typeStatic();
        if (m_baseConstructorCreator.count(typeId)) {
            auto basePtr = m_baseConstructorCreator[typeId]();
            return getDerived<T>(basePtr);
        }
        return nullptr;
    }

    template<typename T>
    QSharedPointer<T> createFromBaseConstructor2() {
        return T::fromBaseConstructor();
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
        stream.setByteOrder(QDataStream::LittleEndian);
        stream >> type;

        if (m_bin.count(type)) {
            return m_bin[type](data);
        }
        return nullptr;
    }

    // Создание из JSON с автоматическим определением типа
    QSharedPointer<RmoBaseMessage> fromJson(const nlohmann::json& j) {
        if (!j.contains("type")) return nullptr;

        int type = j["type"];
        if (m_json.count(type)) {
            return m_json[type](j);
        }
        return nullptr;
    }

    // Создание через базовый конструктор с автоматическим определением типа


    //----------------------------------------------------------------------------------------

    //на светлое будущее(автоматическая генерация функции
    //для создания объектов с использованием базового конструктора
    //---------------------------------
    /*
        template<typename T>
        static QSharedPointer<T> createShared() {
            return QSharedPointer<T>::create();
        }

        template<typename T>
        void registerType(int typeId)
        {
            m_baseConstructorCreator[typeId] = createShared<T>;
        }
        */
    //-----------------------------------

    //БАЗА ОСНОВА---------------------------------------------------------------------------
    void registerType(int type, ByteArrayCreatorFunc bin, JsonCreatorFunc json, BaseConstructorCreatorFunc base) {
        m_bin[type] = bin;
        m_json[type] = json;
        m_baseConstructorCreator[type] = base;
    }

    void registerType(RmoMsgType type, ByteArrayCreatorFunc bin, JsonCreatorFunc json, BaseConstructorCreatorFunc base) {
        registerType(static_cast<int>(type), bin, json, base);
    }

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

    QSharedPointer<RmoBaseMessage> fromBaseConstructor(RmoMsgType type) {

        return fromBaseConstructor(static_cast<int>(type));
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
        // Регистрация типов сообщений
        registerType(
            RmoMsgType::TARGET_PPU,
            RmoMessage_TargetPPU::fromByteArray,
            RmoMessage_TargetPPU::fromJson,
            RmoMessage_TargetPPU::fromBaseConstructor
        );
        registerType(
            RmoMsgType::ENABLE_PPU,
            RmoMessage_EnablePPU::fromByteArray,
            RmoMessage_EnablePPU::fromJson,
            RmoMessage_EnablePPU::fromBaseConstructor
        );
        registerType(
            RmoMsgType::RESET_PPU,
            RmoMessage_ResetPPU::fromByteArray,
            RmoMessage_ResetPPU::fromJson,
            RmoMessage_ResetPPU::fromBaseConstructor
        );
        registerType(
            RmoMsgType::STOP_PPU,
            RmoMessage_StopPPU::fromByteArray,
            RmoMessage_StopPPU::fromJson,
            RmoMessage_StopPPU::fromBaseConstructor
        );
        registerType(
            RmoMsgType::MCFOS_WRITE_DAC_REF,
            RmoMessage_MCFOS_Write_DAC_REF::fromByteArray,
            RmoMessage_MCFOS_Write_DAC_REF::fromJson,
            [](){ return QSharedPointer<RmoMessage_MCFOS_Write_DAC_REF>::create(); }
        );
        registerType(
            RmoMsgType::MCFOS_START,
            RmoMessage_MCFOS_Start::fromByteArray,
            RmoMessage_MCFOS_Start::fromJson,
            [](){ return QSharedPointer<RmoMessage_MCFOS_Start>::create(); }
        );

        registerType<RmoMessage_TargetPPU>();
        registerType<RmoMessage_EnablePPU>();
        registerType<RmoMessage_ResetPPU>();
        registerType<RmoMessage_StopPPU>();
        registerType<RmoMessage_MCFOS_Write_DAC_REF>();
        registerType<RmoMessage_MCFOS_Start>();

        // registerType(...); // Для новых сообщений
    }

};
