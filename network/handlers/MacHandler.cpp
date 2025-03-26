#include "MacHandler.h"

// Обработчик принятых MAC пакетов:
MacHandler::MacHandler(TimeProvider* timeProvider, QObject* parent)
    : QObject(parent), m_timeProvider(timeProvider)
{

}

MacHandler::~MacHandler()
{

}

// Обработчик принятых MAC пакетов:
void MacHandler::messageProccess(const Message &msg)
{

}

// Проверка сокета на наличие принятых MAC пакетов:
void MacHandler::checkReceived()
{

}
