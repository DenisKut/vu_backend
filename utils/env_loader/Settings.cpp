#include "Settings.h"
#include <qcoreapplication.h>

QSettings &appSettings()
{
    static QSettings s(QCoreApplication::applicationDirPath() +"/settings.ini", QSettings::IniFormat);
    return s;
}

AppMode currentMode()
{
    QString mode = appSettings().value("mode/mode").toString();
    if (mode == "VU_REMOTE_ALMAZ")
        return AppMode::VU_REMOTE_ALMAZ;
    return AppMode::VU_LOCALHOST;
}