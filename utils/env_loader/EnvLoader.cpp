#include "EnvLoader.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>

EnvLoader::EnvLoader(const QString& path) : _path(path) {}

const QRegularExpression EnvLoader::NON_HEX_REGEX("[^0-9a-fx]");
const QRegularExpression EnvLoader::NON_HEX_REGEX_MAC("[^0-9a-fA-F]");

bool EnvLoader::load() {
    QFile file(_path);

    // Логирование пути к файлу
    qDebug() << "Loading .env from:" << _path;

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open .env file:" << file.errorString() << "| Path:" << _path;
        return false;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;

        QStringList parts = line.split('=', Qt::SkipEmptyParts);
        if (parts.size() < 2) continue;

        QString key = parts[0].trimmed();
        QString value = parts.mid(1).join('=').trimmed(); // Объединяем оставшиеся части

        // Удаляем кавычки, если есть
        if (value.startsWith('"') && value.endsWith('"')) {
            value = value.mid(1, value.size() - 2);
        }

        _variables.insert(key, value);
    }

    file.close();
    return true;
}

QString EnvLoader::getStr(const QString& key, const QString& defaultValue) const {
    return _variables.value(key, defaultValue);
}

int EnvLoader::getInt(const QString& key, int defaultValue) const {
    QString value = getStr(key);
    if (value.isEmpty()) return defaultValue;

    bool ok;
    int base = 10;
    if (value.startsWith("0x")) {
        base = 16;
        value = value.mid(2);
    } else if (value.startsWith("0b")) {
        base = 2;
        value = value.mid(2);
    }

    int result = value.toInt(&ok, base);
    return ok ? result : defaultValue;
}

quint8 EnvLoader::getUInt8(const QString& key, quint8 defaultValue) const {
    bool ok;
    quint32 value = strToUInt64(getStr(key), &ok);
    return ok ? static_cast<quint8>(value & 0xFF) : defaultValue;
}

quint16 EnvLoader::getUInt16(const QString& key, quint16 defaultValue) const {
    bool ok;
    quint32 value = strToUInt64(getStr(key), &ok);
    return ok ? static_cast<quint16>(value & 0xFFFF) : defaultValue;
}

float EnvLoader::getFloat(const QString& key, float defaultValue) const {
    QString value = getStr(key);
    bool ok;
    float result = value.toFloat(&ok);
    return ok ? result : defaultValue;
}

quint64 EnvLoader::getMacAddressAsUInt64(const QString& key, quint64 defaultValue) const {
    QString value = getStr(key);
    value.remove("0x").remove(NON_HEX_REGEX_MAC);

    if (value.length() != 12) { // MAC должен быть 6 байт (12 hex символов)
        return defaultValue;
    }

    bool ok;
    return value.toULongLong(&ok, 16);
}

quint32 EnvLoader::getUInt32(const QString& key, quint32 defaultValue) const {
    QString value = getStr(key);
    if (value.isEmpty()) return defaultValue;

    bool ok;
    quint32 result = static_cast<quint32>(strToUInt64(value, &ok));
    return ok ? result : defaultValue;
}

quint64 EnvLoader::getUInt64(const QString& key, quint64 defaultValue) const {
    bool ok;
    quint64 result = strToUInt64(getStr(key), &ok);
    return ok ? result : defaultValue;
}

bool EnvLoader::getBool(const QString& key, bool defaultValue) const {
    QString value = getStr(key).toLower();
    if (value == "true" || value == "1") return true;
    if (value == "false" || value == "0") return false;
    return defaultValue;
}

quint64 EnvLoader::strToUInt64(const QString& s, bool* ok) const {
    QString value = s.trimmed().toLower();

    // Удаляем нечисловые префиксы
    value.remove(NON_HEX_REGEX);

    int base = 10;
    if (value.startsWith("0x")) {
        base = 16;
        value = value.mid(2);
    } else if (value.startsWith("0b")) {
        base = 2;
        value = value.mid(2);
    }

    return value.toULongLong(ok, base);
}
QHash<QString, QString> EnvLoader::getVariables() {
    return _variables;
}
