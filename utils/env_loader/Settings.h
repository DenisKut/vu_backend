#pragma once
#include <QSettings>
enum class AppMode
{
	VU_LOCALHOST,
	VU_REMOTE_ALMAZ
};
/// @brief функция получение объекта с параметрами из settings.ini
/// @return QSettings - объект с параметрами настроек
QSettings &appSettings();

/// @brief группа настроек по названию
/// @param gname - название группы
/// @return QVarianMap - объект с настройками по указанной группе
QVariantMap getGroupSettings(const QString &gname);

/// @brief проверка объекта на пустые поля
/// @param checkable QVariantMap - настройки по названию группы
/// @param groupName назание группы (опционально, для вывода)
void checkGroupEmptiness(QVariantMap &checkable, const QString &groupName = QString());

/// @brief Чтение режима работы из настроек
/// @return enum AppMode - режим работы программы
AppMode currentMode();
