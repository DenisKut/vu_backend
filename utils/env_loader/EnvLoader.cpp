#include "envloader.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

EnvLoader::EnvLoader(const QString& path) : _path(path) {}

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

QString EnvLoader::get(const QString& key, const QString& defaultValue) const {
    return _variables.value(key, defaultValue);
}
