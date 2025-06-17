#include "LogHeader.h"

LogHeader::LogHeader(const QString& source, const LogLevel& logLevel)
    : timestampMicroseconds(TimeProvider::instance().getMks()),
    src(source),
    level(logLevel)
{}

LogHeader::LogHeader(qint64 timestamp, const QString& source, LogLevel logLevel)
    : timestampMicroseconds(timestamp),
    src(source),
    level(logLevel)
{}


void LogHeader::serialize(QDataStream& stream) const {

    stream << timestampMicroseconds << src.toUtf8() << static_cast<quint8>(level);
}

void LogHeader::deserialize(QDataStream& stream) {
    //---------------------------------------
    QByteArray src;
    //---------------------------------------

    quint8 levelVal;
    stream >> timestampMicroseconds >> src >> levelVal;
    level = static_cast<LogLevel>(levelVal);

    //----------------------------------------
    this->src = QString::fromUtf8(src);
    //----------------------------------------


}


//сериализация
QDataStream& operator<<(QDataStream& stream, const LogHeader& header) {
    /*
    stream << header.timestampMicroseconds
           << header.src
           << static_cast<quint8>(header.level);
    */
    header.serialize(stream);
    return stream;
}

//десериализация
 QDataStream& operator>>(QDataStream& stream, LogHeader& header) {
     /*
     quint8 level;
    stream >> header.timestampMicroseconds
        >> header.src
        >> level;
    header.level = static_cast<LogLevel>(level);
    */
    header.deserialize(stream);
    return stream;
}
