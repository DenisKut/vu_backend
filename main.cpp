#include <QCoreApplication>
#include <QTextCodec>
#include <QDir>
#include <QFileInfo>
#include <csignal>
// !!! Необязательно !!!
// #ifdef Q_OS_WIN
// #include <windows.h>
// #endif

#include "utils/env_loader/EnvLoader.h"
#include "common/CoreManager.h"
#include "utils/env_loader/Settings.h"
#include "common/test_fftw.cpp"


void performShuttingDown(int sig){
    Q_UNUSED(sig);
    Logger::instance().stop();
    exit(1);
}

int main(int argc, char* argv[]) {
    qRegisterMetaType<QSharedPointer<RmoBaseMessage>>("QSharedPointer<RmoBaseMessage>");
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

    qRegisterMetaType<QAbstractSocket::
            SocketError>("QAbstractSocket::SocketError");

    QCoreApplication a(argc, argv);

    // Инициализация Логгера (обязательно, иначе Логгер не запустится)
    try{
        Logger::instance().initialize(TimeProvider::instance().getMs());
    } catch (const std::exception& e) {
        qCritical() << "Не могу связаться с БД. Нет логирования!." << e.what();
    } catch (...) {
        qCritical() << "Не могу связаться с БД. Неизвестная ошибка.";
    }

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
    AppConfig::instance().init(env);

    // Вывод кириллицы
    qDebug().noquote() << QString::fromUtf8("▐▀▄▀▌ Загружена версия протокола:")
                       << AppConfig::instance().PBU_PROTO_MAJOR_VERSION()
                       << "."
                       << AppConfig::instance().PBU_PROTO_MINOR_VERSION();

    qDebug().noquote() << QString::fromUtf8("Проверка hex-значения:") << Qt::hex
                       << AppConfig::instance().PBU_PROTO_0X01_ID_MANUF();

    qDebug().noquote() << "MAC Destination:" << Qt::hex
                       << AppConfig::instance().ETH_PROTO_MAC_DST();
    qDebug().noquote() << "MAC Source:" << Qt::hex
                       << AppConfig::instance().ETH_PROTO_MAC_SRC();




    // Запуск ядра приложения
    CoreManager core;
    core.initialize();

    //отлов ошибок и сигналов завершения работы программы, чтобы дописать данные в логгер
    for (int sig : {SIGINT, SIGSEGV, SIGTERM}) {
        signal(sig, performShuttingDown);
    }
    test_fftw(); // Проверка работы FFTW
    return a.exec();
}
