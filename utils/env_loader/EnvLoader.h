#ifndef ENVLOADER_H
#define ENVLOADER_H

#include <QHash>
#include <QString>
#include <QRegularExpression>

class EnvLoader {
public:
    EnvLoader(const QString& path = ".env");
    bool load();

    // Базовые методы
    QString getStr(const QString& key, const QString& defaultValue = "") const;
    int getInt(const QString& key, int defaultValue = 0) const;

    // Новые методы для типизированного чтения
    quint8 getUInt8(const QString& key, quint8 defaultValue = 0) const;
    quint16 getUInt16(const QString& key, quint16 defaultValue = 0) const;
    quint32 getUInt32(const QString& key, quint32 defaultValue = 0) const;
    quint64 getUInt64(const QString& key, quint64 defaultValue = 0) const;

    float getFloat(const QString& key, float defaultValue = 0.0f) const;

    bool getBool(const QString& key, bool defaultValue = false) const;

    // Специальные конвертеры
    quint64 getMacAddressAsUInt64(const QString& key, quint64 defaultValue = 0) const;

    QHash<QString, QString> getVariables();

private:
    QString _path;
    QHash<QString, QString> _variables;

    // Вспомогательные методы
    quint64 strToUInt64(const QString& s, bool* ok = nullptr) const;

    //Переменные
    static const QRegularExpression NON_HEX_REGEX_MAC;
    static const QRegularExpression NON_HEX_REGEX;
};

#endif // ENVLOADER_H
