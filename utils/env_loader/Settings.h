#pragma once
#include <QSettings>
enum class AppMode
{
    VU_LOCALHOST,
    VU_REMOTE_ALMAZ
};

QSettings &appSettings();
AppMode currentMode();

