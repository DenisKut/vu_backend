#include <QCoreApplication>
#include <QTextCodec>
#include <QDir>
#include <QDebug>
#include <QFileInfo>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "utils/env_loader/EnvLoader.h"
#include "utils/AppConfig.h"
#include "core/CoreManager.h"

int main(int argc, char* argv[]) {
#ifdef Q_OS_WIN
    // Настройка консоли Windows
    SetConsoleOutputCP(65001); // UTF-8
    SetConsoleCP(65001);
    // Шрифт Consolas для поддержки Unicode
    CONSOLE_FONT_INFOEX fontInfo;
    fontInfo.cbSize = sizeof(fontInfo);
    GetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &fontInfo);
    wcscpy(fontInfo.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &fontInfo);
#endif

    // Глобальная кодировка UTF-8
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    QCoreApplication a(argc, argv);

    // Путь к .env
    QString envPath = QCoreApplication::applicationDirPath() + "/.env";
    envPath = QDir::toNativeSeparators(envPath);

    // Проверка существования файла
    if (!QFileInfo::exists(envPath)) {
        qCritical().noquote() << QString::fromUtf8("Файл не найден:") << envPath;
        return -1;
    }

    // Загрузка .env
    EnvLoader env(envPath);
    if (!env.load()) {
        qCritical().noquote() << QString::fromUtf8("Ошибка загрузки .env!");
        return -1;
    }

    // Инициализация конфига
    AppConfig::instance().init(env.variables());

    // Вывод кириллицы
    qDebug().noquote() << QString::fromUtf8("▐▀▄▀▌ Загружена версия протокола:")
                       << AppConfig::instance().PBU_PROTO_MAJOR_VERSION()
                       << "."
                       << AppConfig::instance().PBU_PROTO_MINOR_VERSION();

    CoreManager core;
    core.initialize();

    return a.exec();
}
