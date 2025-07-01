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

/*
void performShuttingDown(int sig){
	Q_UNUSED(sig);
	//Logger::instance().stop();
	exit(1);
}
*/
int main(int argc, char *argv[])
{

	qRegisterMetaType<QSharedPointer<RmoBaseMessage>>("QSharedPointer<RmoBaseMessage>");
	qRegisterMetaType<LogHeader *>("LogHeader *");

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

	try
	{
		LogManager::instance();
		// Logger::instance().initialize(TimeProvider::instance().getMs());
	}
	catch (const std::exception &e)
	{
		qCritical() << "Ошибка инициализации логгера: " << e.what();
	}
	catch (...)
	{
		qCritical() << "Ошибка инициализации логгера: неизвестная ошибка!";
	}

	// Путь к .env
	QString envPath = QCoreApplication::applicationDirPath() + "/.env";
	envPath = QDir::toNativeSeparators(envPath);

	// Проверка существования файла
	if (!QFileInfo::exists(envPath))
	{
		qCritical().noquote() << QString::fromUtf8("Файл не найден:") << envPath;
		return -1;
	}

	// Загрузка .env
	EnvLoader env(envPath);
	if (!env.load())
	{
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

	// отлов ошибок и сигналов завершения работы программы, чтобы дописать данные в логгер
	/*
	for (int sig : {SIGINT, SIGSEGV, SIGTERM}) {
		signal(sig, performShuttingDown);
	}
	*/
	test_fftw(); // Проверка работы FFTW




//    // Проверка конвертации и сохранения знака
//    //-----------------------------------------------------------------
//    // Конвертация массива QByteArray в массив float
//    QByteArray vecIn; // 0xFFFFFE93 0x000001CA
//    vecIn.append(0x93);
//    vecIn.append(0xFE);
//    vecIn.append(0xFF);
//    vecIn.append(0xFF);
//    vecIn.append(0xCA);
//    vecIn.append(0x01);
//    vecIn.append(QByteArray(1,0));
//    vecIn.append(QByteArray(1,0));
//    QVector<float> vecOut;
//    for (int i = 0; i < vecIn.size(); i += 4)
//    {
//        qint32 word = 0x000000FF & static_cast<qint32>(vecIn[i]);
//        word |= 0x0000FF00 & (static_cast<qint32>(vecIn[i + 1]) << 8);
//        word |= 0x00FF0000 & (static_cast<qint32>(vecIn[i + 2]) << 16);
//        word |= 0xFF000000 & (static_cast<qint32>(vecIn[i + 3]) << 24);
//        vecOut.append(static_cast<float>(word));
//    }
//    qInfo() << "vecOut[0] =" << vecOut[0];
//    qInfo() << "vecOut[1] =" << vecOut[1];
//    //-----------------------------------------------------------------






	return a.exec();
}
