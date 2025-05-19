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
    std::unique_ptr<T> make_unique(Args&&... args) {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
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

    static std::unique_ptr<RmoBaseMessage> fromByteArray(const QByteArray& data) {
        auto msg = std::make_unique<RmoMessage_TargetPPU>();
        QDataStream stream(data);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream >> msg->typeField
               >> msg->azTarget >> msg->anTarget
               >> msg->azTargetVel >> msg->anTargetVel;
        return msg;
    }

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

    static std::unique_ptr<RmoBaseMessage> fromJson(const nlohmann::json& j) {
        auto msg = std::make_unique<RmoMessage_TargetPPU>();
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
};

class RmoMessageFactory {
    public:
        using CreatorFunc = std::function<std::unique_ptr<RmoBaseMessage>(const QByteArray&)>;
        using JsonCreatorFunc = std::function<std::unique_ptr<RmoBaseMessage>(const nlohmann::json&)>;
    
        static RmoMessageFactory& instance() {
            static RmoMessageFactory inst;
            return inst;
        }
    
        void registerType(int type, CreatorFunc bin, JsonCreatorFunc json) {
            m_bin[type] = bin;
            m_json[type] = json;
        }
    
        std::unique_ptr<RmoBaseMessage> fromByteArray(int type, const QByteArray& data) {
            if (m_bin.count(type)) return m_bin[type](data);
            return nullptr;
        }
    
        std::unique_ptr<RmoBaseMessage> fromJson(int type, const nlohmann::json& j) {
            if (m_json.count(type)) return m_json[type](j);
            return nullptr;
        }
    
    private:
        std::unordered_map<int, CreatorFunc> m_bin;
        std::unordered_map<int, JsonCreatorFunc> m_json;
    
        RmoMessageFactory() {
            // Регистрация типов сообщений
            registerType(static_cast<int>(RmoMsgType::TARGET_PPU),
                RmoMessage_TargetPPU::fromByteArray,
                RmoMessage_TargetPPU::fromJson
            );
            // registerType(...); // Для новых сообщений
        }
    };
