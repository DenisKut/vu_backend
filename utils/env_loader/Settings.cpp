#include "Settings.h"

QSettings &appSettings()
{
    static QSettings s("../../settings.ini", QSettings::IniFormat);
    return s;
}
