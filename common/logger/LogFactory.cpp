#include "LogFactory.h"
#include <QIODevice>
#include <QDebug>

std::unique_ptr<LogHeader> LogFactory::createLog(LogType type, QDataStream& stream) {
    auto it = creators.find(type);
    return it != creators.end() ? it->second(stream) : nullptr;
}
