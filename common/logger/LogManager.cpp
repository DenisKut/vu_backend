#include "LogManager.h"
#include "LogSession.h"

std::unique_ptr<LogSession> LogManager::createInstance() {
    return make_unique<LogSession>();
}

LogSession& LogManager::instance() {
    static auto session = createInstance();
    return *session;
}
