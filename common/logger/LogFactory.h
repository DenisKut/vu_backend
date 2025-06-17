#ifndef LOGFACTORY_H
#define LOGFACTORY_H
#include <QDataStream>
#include <functional>
#include <unordered_map>
#include "messageLogs/LogHeader.h"
#include "messageLogs/PrtUtils.h"


class LogFactory {
public:
    template<typename T>
    void registerType(LogType type) {
        creators[type] = [](QDataStream& stream) -> std::unique_ptr<LogHeader> {
            auto obj = make_unique<T>();
            stream >> *obj;
            return stream.status() == QDataStream::Ok ? std::move(obj) : nullptr;
        };
    }

    std::unique_ptr<LogHeader> createLog(LogType type, QDataStream& stream);

private:
    std::unordered_map<LogType, std::function<std::unique_ptr<LogHeader>(QDataStream&)>> creators;
};
#endif // LOGFACTORY_H
