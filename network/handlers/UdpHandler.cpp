#include "UdpHandler.h"
#include <QNetworkDatagram>
#include <QMetaType>
#include <QDebug>
#include <qmath.h>
#include <math.h>
#include <QFile>
#include <QTextStream>
#include "../../common/calculator/Calculator.h"

#define _pi 3.1415926536

UdpHandler::UdpHandler(QObject *parent)
	: QObject(parent), m_socket(nullptr)
{
	// Регистрация QHostAddress для queued connections
    qRegisterMetaType<QHostAddress>("QHostAddress");
}

void UdpHandler::mcfosWrite_DAC_REF(int N, int windowType,
                                    float A, float f0, float df)
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // АДРЕС И ПОРТ НАЗНАЧЕНИЯ НЕ ЗАГРУЖАЮТСЯ ИЗ ФАЙЛА НАСТРОЕК -
    // НУЖНО РАЗОБРАТЬСЯ, В ЧЁМ ПРИЧИНА
    // m_targetAddr = QHostAddress("192.168.2.10");
    // m_targetPort = 49498;
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    // Расчёт оконной функции для излучаемого сигнала
    float *windowDAC = nullptr;
    windowDAC = getWindowTukey(N, 0.15f);

    // ОШИБКА - не удалось создать оконную функцию
    // для излучаемого сигнала
    if (windowDAC == nullptr)
    {
        qInfo() << "ERROR: cannot create window function for MCFOS DAC!";
        return;
    }

    // Расчёт оконной функции для опорного сигнала
    float *windowREF = nullptr;
    switch (windowType)
    {
    case MCFOS_WindowType_Nuttall:
    {
        windowREF = getWindowNuttall(N);
        break;
    }
    case MCFOS_WindowType_BlackmanHarris:
    {
        windowREF = getWindowBlackmanHarris(N);
        break;
    }
    case MCFOS_WindowType_Blackman:
    {
        windowREF = getWindowBlackman(N);
        break;
    }
    default:
    {
        windowREF = nullptr;
        break;
    }
    }

    // ОШИБКА - не удалось создать оконную функцию
    // для опорного сигнала
    if (windowREF == nullptr)
    {
        qInfo() << "ERROR: cannot create window function for MCFOS REF!";
        return;
    }

    // Генерируем массивы излучаемого и опорного сигналов,
    // обрезая разрядность до 16-ти бит и добавляя номера буферов
    //------------------------------------------------------------------------------------
    vecSignalDAC_Re.clear();
    vecSignalDAC_Im.clear();
    vecSignalREF_Re.clear();
    vecSignalREF_Im.clear();
    const float A_REF = 32760.f; // Для буфера REF всегда используем максимальную амплитуду
    float fN = static_cast<float>(N);
    for (int i = 0; i < N; i++)
    {
        float it = static_cast<float>(i);
        float re = cos(0.01 * 2. * _pi * (f0 * it + df * it * it / (fN * 2.))); // Синфазная составляющая
        float im = sin(0.01 * 2. * _pi * (f0 * it + df * it * it / (fN * 2.))); // Квадратурная составляющая

        vecSignalDAC_Re.append(0x00010000 | (0x0000FFFF & static_cast<quint32>(static_cast<qint16>(qRound(A * re * windowDAC[i])))));
        vecSignalDAC_Im.append(0x00020000 | (0x0000FFFF & static_cast<quint32>(static_cast<qint16>(qRound(A * im * windowDAC[i])))));

        vecSignalREF_Re.append(0x00040000 | (0x0000FFFF & static_cast<quint32>(static_cast<qint16>(qRound(A_REF * re * windowREF[i])))));
        vecSignalREF_Im.append(0x00080000 | (0x0000FFFF & static_cast<quint32>(static_cast<qint16>(qRound(-A_REF * im * windowREF[i])))));
    }

    // Освобождаем память, выделенную под оконные функции
    free(windowDAC);
    free(windowREF);

    // * В буфер vecSignalREF_Im пишутся комплексно-сопряжённые значения,
    //   поэтому в формуле есть знак "минус"

    // Зануляем первый и последний элементы массивов,
    // добавляя номера буферов и признаки последнего отсчёта
    vecSignalDAC_Re[0] = 0x00010000;
    vecSignalDAC_Re[N - 1] = 0x01010000;

    vecSignalDAC_Im[0] = 0x00020000;
    vecSignalDAC_Im[N - 1] = 0x01020000;

    vecSignalREF_Re[0] = 0x00040000;
    vecSignalREF_Re[N - 1] = 0x01040000;

    vecSignalREF_Im[0] = 0x00080000;
    vecSignalREF_Im[N - 1] = 0x01080000;
    //------------------------------------------------------------------------------------

    UdpMessage_MCFOS_Cmd mes;

    mes.CMD_BUF_R = 1;
    // Режим работы МЦФОС
    m_mode = 1; // очистка буферов ЦАП и Ref
    sendMessage(mes.serialize());
    QThread::msleep(10);

    mes.CMD_BUF_R = 0;
    // Режим работы МЦФОС
    m_mode = 2; // запись сигнала в буфер ЦАП
    writeSignalToMCFOS(&mes, &vecSignalDAC_Re);
    writeSignalToMCFOS(&mes, &vecSignalDAC_Im);
    // Режим работы МЦФОС
    m_mode = 3; // запись сигнала в буфер REF
    writeSignalToMCFOS(&mes, &vecSignalREF_Re);
    writeSignalToMCFOS(&mes, &vecSignalREF_Im);

    qInfo() << "[MCFOS]: signal has been written to DAC and REF";
}

void UdpHandler::mcfosStart(quint32 isSignalFromDAC,
                            uint16_t periodSize,
                            uint16_t periodCount,
                            uint32_t ampOff,
                            uint16_t pulseOn,
                            uint16_t pulseOff,
                            uint16_t procDelay,
                            uint16_t procDelta,
                            int numRangeCh)
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // АДРЕС И ПОРТ НАЗНАЧЕНИЯ НЕ ЗАГРУЖАЮТСЯ ИЗ ФАЙЛА НАСТРОЕК -
    // НУЖНО РАЗОБРАТЬСЯ, В ЧЁМ ПРИЧИНА
    // m_targetAddr = QHostAddress("192.168.2.10");
    // m_targetPort = 49498;
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    // Формирование заголовка сообщения на МЦФОС:
    //--------------------------------------------------------------------
    UdpMessage_MCFOS_Cmd mes;
    mes.CMD_C_SH = 8;
    mes.CMD_DAC_S = 1;
    mes.CMD_ADC_S = 1;
    mes.PERIOD_SIZE = periodSize - 1;
    mes.PERIOD_COUNT = periodCount - 1;
    mes.AMP_OFF = ampOff - 1;
    mes.PULSE_ON = pulseOn - 2;
    mes.PULSE_OFF = pulseOff - 2;
    mes.PROC_DELAY = procDelay - 1;
    mes.PROC_DELTA = procDelta - 1;

    // Захват сигнала из буфера ЦАП:
    if (isSignalFromDAC != 0)
    {
        mes.CMD_MUX = 2;
    }

    // Начальный адрес чтения DDR памяти:
    mes.DDR_ADDR = 0;

    // Кол-во байт, которое должно быть вычитано из DDR памяти:
    quint32 DDR_LEN;
    if (isCorrEn)
    {
        // При чтении результата корреляционной обработки
        mes.CMD_COR_EN = 1;
        DDR_LEN = ceil(4 * 2 * 4 * numRangeCh * (mes.PERIOD_COUNT + 1) / 1024) * 1024;
    }
    else
    {
        // При чтении сигнальной дорожки АЦП
        DDR_LEN = ceil(2 * 2 * 4 * mes.PERIOD_SIZE * (mes.PERIOD_COUNT + 1) / 1024) * 1024;
    }
    mes.DDR_LEN = DDR_LEN;
    //--------------------------------------------------------------------

    // Режим работы МЦФОС
    m_mode = 4; // Излучение и приём
    QByteArray dataTemp;
    dataTemp.append(mes.serialize());
    //    qInfo() << Qt::endl << "UDP Message MCFOS:" << dataTemp.toHex(' ').toUpper() << Qt::endl;
    sendMessage(dataTemp);

    // Пауза для гарантированного приёма последнего сигнала в пачке:
    float tPauseMs = 1000.0 * static_cast<float>(periodCount) *
                     static_cast<float>(periodSize) * 0.00000001;
    unsigned long pauseMs = static_cast<unsigned long>(tPauseMs);
    qInfo() << "[MCFOS] QThread pauseMs for receive signal =" << pauseMs;
    QThread::msleep(pauseMs);

    //--------------------------------------------------------------------
    // Далее начинаем процесс чтения результата, записанного в DDR память
    //--------------------------------------------------------------------

    // Очищаем буфер, в который будем складывать данные, считанные из DDR
    recDDR.clear();

    qInfo() << "[MCFOS] Read data from DDR:";

    // Формирование заголовка сообщения на МЦФОС:
    //--------------------------------------------------------------------
    UdpMessage_MCFOS_Cmd mesRead;
    mesRead.CMD_DDR_R = 1;
    mcfosAddrDDR = 1;
    mcfosAddrMaxDDR = 8 * numRangeCh * periodCount;
    mesRead.DDR_ADDR = 4 * (mcfosAddrDDR - 1);
    quint32 size = mcfosAddrSizeDDR;
    if (mcfosAddrMaxDDR - mcfosAddrDDR + 1 <= mcfosAddrSizeDDR)
    {
        size = mcfosAddrMaxDDR - mcfosAddrDDR + 1;
    }
    mesRead.DDR_LEN = 4 * size;
    mcfosAddrDDR += size;

    if (isCorrEn)
    {
        // При чтении результата корреляционной обработки
        mesRead.CMD_COR_EN = 1;
    }
    //--------------------------------------------------------------------

    // Режим работы МЦФОС
    m_mode = 6; // Чтение результата из DDR
    sendMessage(mesRead.serialize());

    qInfo() << "TIME BEGIN MCFOS: " << TimeProvider::instance().getMks();
}

void UdpHandler::slot_mcfosWrite_DAC_REF(QSharedPointer<RmoBaseMessage> msg)
{
    qInfo() << "[MCFOS] mcfosWrite_DAC_REF called with message:";
    qInfo() << "m_targetAddr: " << m_targetAddr.toString();
	if (msg)
		msg->info();
	// Здесь можно привести msg к RmoMessage_MCFOS_Write_DAC_REF и использовать параметры
	// auto m = qSharedPointerCast<RmoMessage_MCFOS_Write_DAC_REF>(msg);
	// ...
	auto m = dynamic_cast<RmoMessage_MCFOS_Write_DAC_REF *>(msg.get());
    //    auto m = qSharedPointerCast<RmoMessage_MCFOS_Write_DAC_REF>(msg);

    mcfosWrite_DAC_REF(m->pulseSize,
                       static_cast<int>(m->windowType),
                       m->amlitude,
                       m->f0,
                       m->df);
}

void UdpHandler::slot_mcfosStart(QSharedPointer<RmoBaseMessage> msg)
{
    qInfo() << "[MCFOS] mcfosStart called with message:";
	if (msg)
		msg->info();
	// Здесь можно привести msg к RmoMessage_MCFOS_Start и использовать параметры
	// auto m = qSharedPointerCast<RmoMessage_MCFOS_Start>(msg);
	// ...
	auto m = dynamic_cast<RmoMessage_MCFOS_Start *>(msg.get());
	//    auto m = qSharedPointerCast<RmoMessage_MCFOS_Start>(msg);

    // Записывать ли результаты в файл
    isWriteFile = (m->isWriteFile != 0) ? true : false;

    // Включена ли корреляционная обработка:
    isCorrEn = (m->isCorrelation != 0) ? true : false;

    // Запуск МЦФОС
    mcfosStart(m->isSignalFromDAC,
               m->periodSize,
               m->periodCount,
               m->ampOff,
               m->pulseOn,
               m->pulseOff,
               m->procDelay,
               m->procDelta,
               m->numRangeCh);
}

UdpHandler::~UdpHandler()
{
	stop();
}

void UdpHandler::start(const QString &my_ip, int my_port,
					   const QString &target_ip, int target_port)
{
	if (m_socket)
		return;
	m_port = my_port;
	m_addr = QHostAddress(my_ip);
	m_targetPort = target_port;
	m_targetAddr = QHostAddress(target_ip);
	m_socket = new QUdpSocket(this);
	qDebug() << "Binding UDP socket to" << m_addr << m_port;
	bool bindResult = m_socket->bind(m_addr, m_port,
									 QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
	connect(m_socket, &QUdpSocket::readyRead, this, &UdpHandler::onReadyRead);
	if (bindResult)
	{
		qInfo() << "[UDP] Listening on" << my_ip << ":" << my_port;
	}
	else
	{
		qCritical() << "[UDP] Failed to bind on" << my_ip << ":" << my_port;
	}
}

void UdpHandler::stop()
{
	if (m_socket)
	{
		m_socket->close();
		m_socket->deleteLater();
		m_socket = nullptr;
	}
}

void UdpHandler::sendMessage(const QByteArray &data)
{
	if (!m_socket)
	{
		qWarning("[UdpHandler] sendMessage: m_socket is nullptr");
		return;
	}
#if defined(QT_DEBUG) || defined(TRACE)
	QStringList traceParts;
#ifdef QT_DEBUG
	traceParts << "[UDP] Sending datagram";
#endif
#ifdef TRACE
	traceParts << "to" << m_targetAddr.toString() + ":" + QString::number(m_targetPort)
			   << ", size:" + QString::number(data.size());
#endif
	qInfo().noquote() << traceParts.join(" ");
#endif
	qint64 written = m_socket->writeDatagram(data, m_targetAddr, m_targetPort);
	if (written != data.size())
	{
		qWarning("[UdpHandler] sendMessage: not all data written (%lld/%d)", written, data.size());
	}
}

void UdpHandler::onReadyRead()
{
	while (m_socket && m_socket->hasPendingDatagrams())
	{
		QByteArray datagram;
		datagram.resize(int(m_socket->pendingDatagramSize()));
		QHostAddress sender;
		quint16 senderPort;
		m_socket->readDatagram(datagram.data(), datagram.size(),
							   &sender, &senderPort);
#if defined(QT_DEBUG) || defined(TRACE)
		QString logLine;
#ifdef QT_DEBUG
		logLine += "[UDP] Received datagram ";
#endif
#ifdef TRACE
		logLine += "from " + sender.toString() + ":" + QString::number(senderPort) + ", size:" + QString::number(datagram.size());
#endif
		qInfo().noquote() << logLine;
#endif
		processIncomingDatagram(datagram, sender, senderPort); // Вызов разбора
		emit packetReceived(datagram, sender, senderPort);
	}
}

void UdpHandler::writeSignalToMCFOS(UdpMessage_MCFOS_Cmd *mes,
									QVector<quint32> vecSignalDAC[])
{
	quint32 vecWordsCount = vecSignalDAC->size();
	quint32 num_words_in_packet = 128;
	quint32 packets_count = static_cast<quint32>(vecWordsCount) / num_words_in_packet;
	quint32 last = static_cast<quint32>(vecWordsCount) % num_words_in_packet;
	quint32 buf_size = (packets_count > 0) ? num_words_in_packet : (last > 0) ? last
																			  : 0;

	//    qInfo() << "num_words_in_packet =" << num_words_in_packet;
	//    qInfo() << "vecSignalDAC_Re.size() =" << vecSignalDAC_Re.size();
	//    qInfo() << "vecWordsCount =" << vecWordsCount;
	//    qInfo() << "packets_count =" << packets_count;
	//    qInfo() << "last =" << last;
	//    qInfo() << "buf_size =" << buf_size;
	if (buf_size > 0)
	{
		for (quint32 i = 0; i < packets_count; i++)
		{
			QByteArray dataHeader, dataBuf;
			mes->BUF_SIZE = buf_size;
			dataHeader.append(mes->serialize());
			dataBuf.append(dataHeader);
			for (quint32 k = 0; k < num_words_in_packet; k++)
			{
				quint32 word = vecSignalDAC->at(k + i * num_words_in_packet);
				dataBuf.append(static_cast<quint8>(word >> 0));
				dataBuf.append(static_cast<quint8>(word >> 8));
				dataBuf.append(static_cast<quint8>(word >> 16));
				dataBuf.append(static_cast<quint8>(word >> 24));
			}
			sendMessage(dataBuf);
			QThread::msleep(10);
			//            qInfo() << "dataBuf =" << Qt::endl << dataBuf.toHex(' ').toUpper();
		}
		// Если есть остаток, то отправляем его в дополнительном пакете
		if (last > 0)
		{
			quint32 buf_size = last;
			QByteArray dataHeader, dataBuf;
			mes->BUF_SIZE = buf_size;
			dataHeader.append(mes->serialize());
			dataBuf.append(dataHeader);
			for (quint32 k = 0; k < last; k++)
			{
				quint32 word = vecSignalDAC->at(k + packets_count * num_words_in_packet);
				dataBuf.append(static_cast<quint8>(word >> 0));
				dataBuf.append(static_cast<quint8>(word >> 8));
				dataBuf.append(static_cast<quint8>(word >> 16));
				dataBuf.append(static_cast<quint8>(word >> 24));
			}
			sendMessage(dataBuf);
			QThread::msleep(10);
			//            qInfo() << "dataBuf_last =" << Qt::endl << dataBuf.toHex(' ').toUpper();
		}
	}
}

/// Генерация окна Натталла (4-терм)
float *UdpHandler::getWindowNuttall(int length)
{
	if (length <= 0)
		return nullptr;

	float *window = (float *)malloc(length * sizeof(float));
	if (!window)
		return nullptr;

	const float a0 = 0.3635819f;
	const float a1 = 0.4891775f;
	const float a2 = 0.1365995f;
	const float a3 = 0.0106411f;

	for (int n = 0; n < length; n++)
	{
		float angle = 2.0f * (float)M_PI * n / (length - 1);
		window[n] = a0 - a1 * cosf(angle) + a2 * cosf(2 * angle) - a3 * cosf(3 * angle);
	}
	return window;
}

/// Генерация окна Блэкмана-Харриса (4-терм)
float *UdpHandler::getWindowBlackmanHarris(int length)
{
	if (length <= 0)
		return nullptr;

	float *window = (float *)malloc(length * sizeof(float));
	if (!window)
		return nullptr;

	const float a0 = 0.35875f;
	const float a1 = 0.48829f;
	const float a2 = 0.14128f;
	const float a3 = 0.01168f;

	for (int n = 0; n < length; n++)
	{
		float angle = 2.0f * (float)M_PI * n / (length - 1);
		window[n] = a0 - a1 * cosf(angle) + a2 * cosf(2 * angle) - a3 * cosf(3 * angle);
	}
	return window;
}

/// Генерация окна Блэкмана (3-терм)
float *UdpHandler::getWindowBlackman(int length)
{
	if (length <= 0)
		return nullptr;

	float *window = (float *)malloc(length * sizeof(float));
	if (!window)
		return nullptr;

	const float a0 = 0.42f;
	const float a1 = 0.5f;
	const float a2 = 0.08f;

	for (int n = 0; n < length; n++)
	{
		float angle = 2.0f * (float)M_PI * n / (length - 1);
		window[n] = a0 - a1 * cosf(angle) + a2 * cosf(2 * angle);
	}
	return window;
}

/// Генерация окна Тьюки
float *UdpHandler::getWindowTukey(int length, const float alpha) {
    if (length <= 0)
        return nullptr;

    float* window = static_cast<float*>(malloc(length * sizeof(float)));
    if (!window)
        return nullptr;

    // Обработка частных случаев
    if (length == 1) {
        window[0] = 1.0f;
        return window;
    }

    const int N = length - 1;  // Для правильной индексации (0..N)
    const float width = alpha * N / 2.0f;  // Ширина "плеча" окна

    for (int i = 0; i < length; ++i) {
        if (i < width) {
            // Возрастающая часть (левое плечо)
            float phase = static_cast<float>(i) / width - 1.0f;
            window[i] = 0.5f * (1.0f + cosf(static_cast<float>(M_PI) * phase));
        } else if (i > N - width) {
            // Убывающая часть (правое плечо)
            float phase = static_cast<float>(i - N) / width + 1.0f;
            window[i] = 0.5f * (1.0f + cosf(static_cast<float>(M_PI) * phase));
        } else {
            // Центральная часть (константа)
            window[i] = 1.0f;
        }
    }

    return window;
}

void UdpHandler::mcfosReadDDR(const QByteArray &data)
{
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// АДРЕС И ПОРТ НАЗНАЧЕНИЯ НЕ ЗАГРУЖАЮТСЯ ИЗ ФАЙЛА НАСТРОЕК -
	// НУЖНО РАЗОБРАТЬСЯ, В ЧЁМ ПРИЧИНА
	// m_targetAddr = QHostAddress("192.168.2.10");
	// m_targetPort = 49498;
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	// Складываем прочитанную порцию данных в буфер
	recDDR.append(data);
//    qInfo() << "[MCFOS] Read data from DDR:" << data.size() << "bytes";

	// Пока не дочитали память до конца
	if (mcfosAddrDDR <= mcfosAddrMaxDDR)
	{
//        qInfo() << "[MCFOS] Send message to MCFOS for read block data, DDR address:"
//                << 4 * (mcfosAddrDDR - 1);
		// Формирование заголовка сообщения на МЦФОС:
		//--------------------------------------------------------------------
		UdpMessage_MCFOS_Cmd mesRead;
		mesRead.CMD_DDR_R = 1;
		mesRead.DDR_ADDR = 4 * (mcfosAddrDDR - 1);
		quint32 size = mcfosAddrSizeDDR;
		if (mcfosAddrMaxDDR - mcfosAddrDDR + 1 <= mcfosAddrSizeDDR)
		{
			size = mcfosAddrMaxDDR - mcfosAddrDDR + 1;
		}
		mesRead.DDR_LEN = 4 * size;
        mcfosAddrDDR += size;
		//--------------------------------------------------------------------

		sendMessage(mesRead.serialize());
	}
	// Дочитали память до конца
	else
	{
        qInfo() << "[MCFOS] All data from DDR has read";

        // Вызываем обработчик принятых данных в Calculator
        QMetaObject::invokeMethod(
            Calculator::instance(),
            [this]() { Calculator::instance()->recDataMCFOS(this->recDDR); },
            Qt::QueuedConnection
        );
	}
}

void UdpHandler::processIncomingDatagram(const QByteArray &data,
										 const QHostAddress &sender,
										 quint16 senderPort)
{
	/// @warning ЗДЕСЬ НУЖНО ДОБАВЛЯТЬ ПЕРЕБОР СООБЩЕНИЙ и ДОБАВЛЕНИЕ ПАРАМЕТРОВ
	/// В ФУНКЦИЮ!!!!!!!!!!!
	// НУЖНО ДОБАВИТЬ ПРОВЕРКУ НА ПРИНЯТОЕ СООБЩЕНИЕ ОТ МЦФОС, ЧТОБЫ НЕ
	// ВЫЗЫВАТЬ ЭТУ ФУНКЦИЮ ПО ПРИЁМУ СООБЩЕНИЙ ОТ ДРУГИХ АБОНЕНТОВ.
	// ПРИЧЁМ, У НАС БУДУТ ДВА МЦФОС С РАЗНЫМИ IP АДРЕСАМИ.
	mcfosReadDDR(data);

	Q_UNUSED(data);
	Q_UNUSED(sender);
	Q_UNUSED(senderPort);
	// Здесь разбор пакета, логика обработки, парсинг и т.д.
	// Пример: просто hex :)

#if defined(QT_DEBUG) || defined(TRACE)
	QStringList traceParts;
#ifdef QT_DEBUG
	traceParts << "[UDP] Parsed datagram";
#endif
#ifdef TRACE
	traceParts << "from" << sender.toString() + ":" + QString::number(senderPort)
			   << ":" + data.toHex(' ').toUpper();
#endif
	qDebug().noquote() << traceParts.join(" ");
#endif
	// TODO: добавить реальный разбор
}

void UdpHandler::sendCustomPacket(const QByteArray &payload,
								  const QHostAddress &addr,
								  quint16 port)
{
	Q_UNUSED(addr);
	Q_UNUSED(port);
	// Здесь формирование пакета (например, добавление заголовка НА БУДУЩЕЕ)
	QByteArray packet = payload; // Пример: без изменений
	sendMessage(packet);
#if defined(QT_DEBUG) || defined(TRACE)
	QStringList traceParts;
#ifdef QT_DEBUG
	traceParts << "[UDP] Custom packet sent";
#endif
#ifdef TRACE
	traceParts << "to" << addr.toString() + ":" + QString::number(port)
			   << ", size:" + QString::number(packet.size());
#endif
	qDebug().noquote() << traceParts.join(" ");
#endif
}
