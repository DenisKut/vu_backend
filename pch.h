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

}
#define CLASS_NAME Utils::demangleClassName(typeid(*this).name())

//#define CLASS_NAME QString(metaObject()->className())

#endif
