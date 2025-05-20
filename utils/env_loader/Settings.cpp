#include "Settings.h"
#include <qcoreapplication.h>

QSettings &appSettings()
{
    static QSettings s(QCoreApplication::applicationDirPath() +"/settings.ini", QSettings::IniFormat);
    return s;
}

QVariantMap getGroupSettings(const QString &groupName)
{
    QVariantMap map;
    QSettings &settings = appSettings();

    settings.beginGroup(groupName);
    QStringList keys = settings.childKeys();
    for (const QString &key : keys) {
        map.insert(key, settings.value(key));
    }
    settings.endGroup();
    checkGroupEmptiness(map, groupName);
    return map;
}

void checkGroupEmptiness(QVariantMap &checkable, const QString &groupName)
{
    bool allValid = true;
    QStringList missingKeys;

    for (QVariantMap::iterator it = checkable.begin(); it != checkable.end(); ++it) {
        if (it.value().toString().trimmed().isEmpty()) {
            allValid = false;
            missingKeys << it.key();
            it.value() = QString();
        }
    }

    if (!allValid) {
        QString groupPrefix = groupName.isEmpty() ? "" : "[" + groupName + "] ";
        qWarning() << "\n\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
        << "!  При чтении параметров группы "<< groupPrefix << "были найдены пустые поля:\n" 
        << "!  "<< missingKeys.join(", ")<< "\n"
        << "!  Продолжаем... действуйте с осторожностью на свой страх и риск\n"
        << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
    }
}


AppMode currentMode()
{
    QString mode = appSettings().value("mode/mode").toString();
    if (mode == "VU_REMOTE_ALMAZ")
        return AppMode::VU_REMOTE_ALMAZ;
    return AppMode::VU_LOCALHOST;
}

