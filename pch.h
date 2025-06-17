//Прекомпилированные заголовки, включаются везде
// #include <QThread>
// #include <QMutex>
// #include <QWaitCondition>
// #include <QQueue>
// #include <QSqlDatabase>                     всё это из Логгера
// #include <QSqlQuery>
// #include <QDateTime>
// #include <QString>
// #include <atomic>
// #include <vector>
// защитник от множественногот включения
#ifndef PCH_H
#define PCH_H
#include "utils/time_provider/TimeProvider.h"
#include "utils/AppConfig.h"

#include <QCoreApplication>
#include <QObject>
#include <QMetaObject>
#include <QThread>
#include <memory>
#include <QDebug>
#include <QMutex>
#include <QFile>
#include <QDataStream>
#include <QString>

//Если хочется без указания типа класса, где был вызов, передавать *this в typeid

#include "common/logger/LoggerMacros.h"


namespace Utils {
    QString demangleClassName(const char* name);

   // Порядок байт в приложении
    constexpr QDataStream::ByteOrder ByteOrder = QDataStream::LittleEndian;

}
#define CLASS_NAME Utils::demangleClassName(typeid(*this).name())

/**
 * @brief Преобразует enum class в базовый целочисленный тип
 * @tparam E Тип перечисления
 * @param e Значение перечисления
 * @return Базовое целочисленное значение
 */
template <typename E>
constexpr auto to_underlying(E e) -> typename std::underlying_type<E>::type {
    return static_cast<typename std::underlying_type<E>::type>(e);
}

#endif
