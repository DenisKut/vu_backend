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
    TARGET_PPU = 1
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
            >> msg->azTarget >> msg->anTarget
            >> msg->azTargetVel >> msg->anTargetVel;
        return msg;
    }

/*
    static QSharedPointer<RmoBaseMessage> fromByteArray(const QByteArray& data) {
        auto msg = QSharedPointer<RmoMessage_TargetPPU>::create();
        QDataStream stream(data);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream >> msg->typeField
            >> msg->azPPU >> msg->anPPU
            >> msg->azC0 >> msg->azC1 >> msg->azC2
            >> msg->anC0 >> msg->anC1 >> msg->anC2
            >> msg->azTarget >> msg->anTarget
            >> msg->azTargetVel >> msg->anTargetVel
            >> msg->isRemote >> msg->counter;
        return msg;
    }
    */
    // --- JSON сериализация ---
    nlohmann::json toJson() const override {
        return {
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

    RmoMessage_TargetPPU()
    {
        azPPU = AppConfig::instance().getModulePPU_az(AppConfig::instance().ModulePPU_num_RLMN_1);
        anPPU = AppConfig::instance().getModulePPU_an(AppConfig::instance().ModulePPU_num_RLMN_1);
        azTarget = AppConfig::instance().getModulePPU_azTarget(AppConfig::instance().ModulePPU_num_RLMN_1);
        anTarget = AppConfig::instance().getModulePPU_anTarget(AppConfig::instance().ModulePPU_num_RLMN_1);
        azTargetVel = AppConfig::instance().getModulePPU_azTargetVel(AppConfig::instance().ModulePPU_num_RLMN_1);
        anTargetVel = AppConfig::instance().getModulePPU_anTargetVel(AppConfig::instance().ModulePPU_num_RLMN_1);
        isRemote = static_cast<uint8_t>(AppConfig::instance().getModulePPU_isRemote(AppConfig::instance().ModulePPU_num_RLMN_1));
    }

    static QSharedPointer<RmoMessage_TargetPPU> fromBaseConstructor()
    {
        return QSharedPointer<RmoMessage_TargetPPU>::create();
    }
    //----------------------------------------------------------------------

    void info() const override
    {
        //qInfo() << azC0 << " " << azC1 << " " << azC2 << " ";
        qInfo() << "AZtarget: " << azTarget << "AnTarget: " << anTarget;
        qInfo() << "AZC0 " << azC0;
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
        if (data.size() < sizeof(int32_t)) return nullptr;

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


        registerType
            (
                RmoMsgType::TARGET_PPU,
                RmoMessage_TargetPPU::fromByteArray,
                RmoMessage_TargetPPU::fromJson,
                RmoMessage_TargetPPU::fromBaseConstructor
            );


        registerType<RmoMessage_TargetPPU>();

        // registerType(...); // Для новых сообщений
    }

};
