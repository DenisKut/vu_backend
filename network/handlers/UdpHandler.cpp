#include "UdpHandler.h"
#include <QNetworkDatagram>
#include <QMetaType>
#include <QDebug>
#include <qmath.h>
#include <math.h>
#include <QFile>
#include <QTextStream>

#define _pi 3.1415926536

UdpHandler::UdpHandler(QObject* parent)
    : QObject(parent), m_socket(nullptr)
{
    // Регистрация QHostAddress для queued connections
    qRegisterMetaType<QHostAddress>("QHostAddress");
}

void UdpHandler::mcfosWrite_DAC_REF(QSharedPointer<RmoBaseMessage> msg)
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // АДРЕС И ПОРТ НАЗНАЧЕНИЯ НЕ ЗАГРУЖАЮТСЯ ИЗ ФАЙЛА НАСТРОЕК -
    // НУЖНО РАЗОБРАТЬСЯ, В ЧЁМ ПРИЧИНА
    m_targetAddr = QHostAddress("192.168.2.10");
    m_targetPort = 49498;
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    qDebug() << "[MCFOS] mcfosWrite_DAC_REF called with message:";
    if (msg) msg->info();
    // Здесь можно привести msg к RmoMessage_MCFOS_Write_DAC_REF и использовать параметры
    // auto m = qSharedPointerCast<RmoMessage_MCFOS_Write_DAC_REF>(msg);
    // ...
    auto m = dynamic_cast<RmoMessage_MCFOS_Write_DAC_REF *>(msg.get());
//    auto m = qSharedPointerCast<RmoMessage_MCFOS_Write_DAC_REF>(msg);

    int N = m->pulseSize; // Длина импульса

    // Расчёт оконной функции
    float *window = nullptr;
    switch (static_cast<int>(m->windowType)) {
    case MCFOS_WindowType_Nuttall: {
        window = getWindowNuttall(N);
        break;
    }
    case MCFOS_WindowType_BlackmanHarris: {
        window = getWindowBlackmanHarris(N);
        break;
    }
    case MCFOS_WindowType_Blackman: {
        window = getWindowBlackman(N);
        break;
    }
    default: {
        window = nullptr;
        break;
    }
    }

    // ОШИБКА - не удалось создать оконную функцию
    if (window == nullptr) {
        qInfo() << "ERROR: cannot create window function for MCFOS!";
        return;
    }

    // Основные параметры для расчёта сигнала
    float A = m->amlitude;
    float f0 = m->f0;
    float df = m->df;

    // Генерируем массивы излучаемого и опорного сигналов,
    // обрезая разрядность до 16-ти бит и добавляя номера буферов
    //------------------------------------------------------------------------------------
    vecSignalDAC_Re.clear();
    vecSignalDAC_Im.clear();
    vecSignalREF_Re.clear();
    vecSignalREF_Im.clear();
    float fN = static_cast<float>(N);
    for (int i = 0; i < N; i++)
    {
        float it = static_cast<float>(i);
        float re = A * cos(0.01 * 2. * _pi * (f0 * it + df * it * it / (fN * 2.))); // Синфазная составляющая
        float im = A * sin(0.01 * 2. * _pi * (f0 * it + df * it * it / (fN * 2.))); // Квадратурная составляющая

        vecSignalDAC_Re.append(0x00010000 | (0x0000FFFF & static_cast<quint32>(static_cast<qint16>(qRound(re)))));
        vecSignalDAC_Im.append(0x00020000 | (0x0000FFFF & static_cast<quint32>(static_cast<qint16>(qRound(im)))));

        vecSignalREF_Re.append(0x00040000 | (0x0000FFFF & static_cast<quint32>(static_cast<qint16>(qRound(re * window[i])))));
        vecSignalREF_Im.append(0x00080000 | (0x0000FFFF & static_cast<quint32>(static_cast<qint16>(qRound(-im * window[i])))));
    }
    // * В буфер vecSignalREF_Im пишутся комплексно-сопряжённые значения,
    //   поэтому в формуле есть знак "минус"

    // Зануляем первый и последний элементы массивов,
    // добавляя номера буферов и признаки последнего отсчёта
    vecSignalDAC_Re[0]      = 0x00010000;
    vecSignalDAC_Re[N - 1]  = 0x01010000;

    vecSignalDAC_Im[0]      = 0x00020000;
    vecSignalDAC_Im[N - 1]  = 0x01020000;

    vecSignalREF_Re[0]      = 0x00040000;
    vecSignalREF_Re[N - 1]  = 0x01040000;

    vecSignalREF_Im[0]      = 0x00080000;
    vecSignalREF_Im[N - 1]  = 0x01080000;
    //------------------------------------------------------------------------------------

    UdpMessage_MCFOS_Cmd mes;

    mes.CMD_BUF_R = 1;
    // Режим работы МЦФОС
    m_mode = 1; // очистка буферов ЦАП и Ref
    sendMessage(mes.serialize());

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

void UdpHandler::mcfosStart(QSharedPointer<RmoBaseMessage> msg)
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // АДРЕС И ПОРТ НАЗНАЧЕНИЯ НЕ ЗАГРУЖАЮТСЯ ИЗ ФАЙЛА НАСТРОЕК -
    // НУЖНО РАЗОБРАТЬСЯ, В ЧЁМ ПРИЧИНА
    m_targetAddr = QHostAddress("192.168.2.10");
    m_targetPort = 49498;
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    qDebug() << "[MCFOS] mcfosStart called with message:";
    if (msg) msg->info();
    // Здесь можно привести msg к RmoMessage_MCFOS_Start и использовать параметры
    // auto m = qSharedPointerCast<RmoMessage_MCFOS_Start>(msg);
    // ...
    auto m = dynamic_cast<RmoMessage_MCFOS_Start *>(msg.get());
//    auto m = qSharedPointerCast<RmoMessage_MCFOS_Start>(msg);

    quint32 isCorrelation = m->isCorrelation;
    quint32 isSignalFromDAC = m->isSignalFromDAC;
    quint32 isWriteFile = m->isWriteFile;

    // Формирование заголовка сообщения на МЦФОС:
    //--------------------------------------------------------------------
    UdpMessage_MCFOS_Cmd mes;
    mes.CMD_DAC_S = 1;
    mes.CMD_ADC_S = 1;
    mes.PERIOD_SIZE = m->periodSize - 1;
    mes.PERIOD_COUNT = m->periodCount - 1;
    mes.AMP_OFF = m->ampOff - 1;
    mes.PULSE_ON = m->pulseOn - 2;
    mes.PULSE_OFF = m->pulseOff - 2;
    mes.PROC_DELAY = m->procDelay - 1;
    mes.PROC_DELTA = m->procDelta - 1;

    // Захват сигнала из буфера ЦАП:
    if (isSignalFromDAC != 0) {
        mes.CMD_MUX = 2;
    }

    // Начальный адрес чтения DDR памяти:
    mes.DDR_ADDR = 0;

    // Кол-во байт, которое должно быть вычитано из DDR памяти:
    quint32 DDR_LEN;
    if (isCorrelation == 0) {
        // При чтении сигнальной дорожки АЦП
        DDR_LEN = ceil(2 * 2 * 4 * mes.PERIOD_SIZE * (mes.PERIOD_COUNT + 1) / 1024) * 1024;
    } else {
        // При чтении результата корреляционной обработки
        mes.CMD_COR_EN = 1;
        DDR_LEN = ceil(4 * 2 * 4 * 100 * (mes.PERIOD_COUNT + 1) / 1024) * 1024;
    }
    mes.DDR_LEN = DDR_LEN;
    //--------------------------------------------------------------------

    // Режим работы МЦФОС
    m_mode = 4; // излучение и приём
    QByteArray dataTemp;
    dataTemp.append(mes.serialize());
//    qInfo() << Qt::endl << "UDP Message MCFOS:" << dataTemp.toHex(' ').toUpper() << Qt::endl;
    sendMessage(dataTemp);

    // Пауза для гарантированного приёма последнего сигнала в пачке:
    float tPauseMs = 1000.0 * static_cast<float>(m->periodCount) * \
                     static_cast<float>(m->periodSize) * 0.00000001;
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
    mcfosAddrMaxDDR = 8 * 100 * m->periodCount;
    mesRead.DDR_ADDR = 4 * (mcfosAddrDDR - 1);
    quint32 size = mcfosAddrSizeDDR;
    if (mcfosAddrMaxDDR - mcfosAddrDDR + 1 <= mcfosAddrSizeDDR) {
        size = mcfosAddrMaxDDR - mcfosAddrDDR + 1;
    }
    mesRead.DDR_LEN = 4 * size;
    mcfosAddrDDR += size;

    if (isCorrelation == 0) {
        // При чтении сигнальной дорожки АЦП
        DDR_LEN = ceil(2 * 2 * 4 * mes.PERIOD_SIZE * (mes.PERIOD_COUNT + 1) / 1024) * 1024;
    } else {
        // При чтении результата корреляционной обработки
        mes.CMD_COR_EN = 1;
        DDR_LEN = ceil(4 * 2 * 4 * 100 * (mes.PERIOD_COUNT + 1) / 1024) * 1024;
    }
    //--------------------------------------------------------------------

    // Режим работы МЦФОС
    m_mode = 6; // чтение результата из DDR
    sendMessage(mesRead.serialize());
}

UdpHandler::~UdpHandler()
{
    stop();
}

void UdpHandler::start(const QString& my_ip, int my_port,
                       const QString& target_ip, int target_port)
{
    if (m_socket) return;
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    m_port = 49499;//my_port; // ИНАЧЕ ПОРТ ИСТОЧНИКА ПРИНИМАЕТ СЛУЧАЙНОЕ ЗНАЧЕНИЕ
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    m_addr = QHostAddress(my_ip);
    m_targetPort = target_port;
    m_targetAddr = QHostAddress(target_ip);
    m_socket = new QUdpSocket(this);
    bool bindResult = m_socket->bind(m_targetAddr, m_port,
                   QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    connect(m_socket, &QUdpSocket::readyRead, this, &UdpHandler::onReadyRead);
    if (bindResult) {
        qInfo() << "[UDP] Listening on" << my_ip << ":" << my_port;
    } else {
        qCritical() << "[UDP] Failed to bind on" << my_ip << ":" << my_port;
    }
}

void UdpHandler::stop()
{
    if (m_socket) {
        m_socket->close();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
}

void UdpHandler::sendMessage(const QByteArray& data)
{
    if (!m_socket) {
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
    if (written != data.size()) {
        qWarning("[UdpHandler] sendMessage: not all data written (%lld/%d)", written, data.size());
    }
}

void UdpHandler::onReadyRead()
{
    while (m_socket && m_socket->hasPendingDatagrams()) {
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
        logLine += "from " + sender.toString()
                + ":" + QString::number(senderPort)
                + ", size:" + QString::number(datagram.size());
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
    quint32 buf_size = (packets_count > 0) ? num_words_in_packet : (last > 0) ? last : 0;

//    qInfo() << "num_words_in_packet =" << num_words_in_packet;
//    qInfo() << "vecSignalDAC_Re.size() =" << vecSignalDAC_Re.size();
//    qInfo() << "vecWordsCount =" << vecWordsCount;
//    qInfo() << "packets_count =" << packets_count;
//    qInfo() << "last =" << last;
//    qInfo() << "buf_size =" << buf_size;
    if (buf_size > 0) {
        for (quint32 i = 0; i < packets_count; i++) {
            QByteArray dataHeader, dataBuf;
            mes->BUF_SIZE = buf_size;
            dataHeader.append(mes->serialize());
            dataBuf.append(dataHeader);
            for (quint32 k = 0; k < num_words_in_packet; k++) {
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
        if (last > 0) {
            quint32 buf_size = last;
            QByteArray dataHeader, dataBuf;
            mes->BUF_SIZE = buf_size;
            dataHeader.append(mes->serialize());
            dataBuf.append(dataHeader);
            for (quint32 k = 0; k < last; k++) {
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
    if (length <= 0) return nullptr;

    float* window = (float*)malloc(length * sizeof(float));
    if (!window) return nullptr;

    const float a0 = 0.3635819f;
    const float a1 = 0.4891775f;
    const float a2 = 0.1365995f;
    const float a3 = 0.0106411f;

    for (int n = 0; n < length; n++) {
        float angle = 2.0f * (float)M_PI * n / (length - 1);
        window[n] = a0
                  - a1 * cosf(angle)
                  + a2 * cosf(2 * angle)
                  - a3 * cosf(3 * angle);
    }
    return window;
}

/// Генерация окна Блэкмана-Харриса (4-терм)
float *UdpHandler::getWindowBlackmanHarris(int length)
{
    if (length <= 0) return nullptr;

    float* window = (float*)malloc(length * sizeof(float));
    if (!window) return nullptr;

    const float a0 = 0.35875f;
    const float a1 = 0.48829f;
    const float a2 = 0.14128f;
    const float a3 = 0.01168f;

    for (int n = 0; n < length; n++) {
        float angle = 2.0f * (float)M_PI * n / (length - 1);
        window[n] = a0
                  - a1 * cosf(angle)
                  + a2 * cosf(2 * angle)
                  - a3 * cosf(3 * angle);
    }
    return window;
}

/// Генерация окна Блэкмана (3-терм)
float *UdpHandler::getWindowBlackman(int length)
{
    if (length <= 0) return nullptr;

    float* window = (float*)malloc(length * sizeof(float));
    if (!window) return nullptr;

    const float a0 = 0.42f;
    const float a1 = 0.5f;
    const float a2 = 0.08f;

    for (int n = 0; n < length; n++) {
        float angle = 2.0f * (float)M_PI * n / (length - 1);
        window[n] = a0
                  - a1 * cosf(angle)
                  + a2 * cosf(2 * angle);
    }
    return window;
}

void UdpHandler::mcfosReadDDR(const QByteArray& data) {
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // АДРЕС И ПОРТ НАЗНАЧЕНИЯ НЕ ЗАГРУЖАЮТСЯ ИЗ ФАЙЛА НАСТРОЕК -
    // НУЖНО РАЗОБРАТЬСЯ, В ЧЁМ ПРИЧИНА
    m_targetAddr = QHostAddress("192.168.2.10");
    m_targetPort = 49498;
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    // Складываем прочитанную порцию данных в буфер
    recDDR.append(data);
//    qInfo() << "[MCFOS] Read data from DDR:" << data.size() << "bytes";

    // Пока не дочитали память до конца
    if (mcfosAddrDDR <= mcfosAddrMaxDDR) {
//        qInfo() << "[MCFOS] Send message to MCFOS for read block data, DDR address:"
//                << 4 * (mcfosAddrDDR - 1);
        // Формирование заголовка сообщения на МЦФОС:
        //--------------------------------------------------------------------
        UdpMessage_MCFOS_Cmd mesRead;
        mesRead.CMD_DDR_R = 1;
        mesRead.DDR_ADDR = 4 * (mcfosAddrDDR - 1);
        quint32 size = mcfosAddrSizeDDR;
        if (mcfosAddrMaxDDR - mcfosAddrDDR + 1 <= mcfosAddrSizeDDR) {
            size = mcfosAddrMaxDDR - mcfosAddrDDR + 1;
        }
        mesRead.DDR_LEN = 4 * size;
        mcfosAddrDDR += size;

//        if (mcfosAddrMaxDDR - mcfosAddrDDR + 1 > mcfosAddrSizeDDR) {
//            mesRead.DDR_LEN = 4 * mcfosAddrSizeDDR;
//            mcfosAddrDDR += mcfosAddrSizeDDR;
//        } else {
//            mesRead.DDR_LEN = 4 * (mcfosAddrMaxDDR - mcfosAddrDDR + 1);
//            mcfosAddrDDR += mcfosAddrMaxDDR - mcfosAddrDDR + 1;
//        }
        //--------------------------------------------------------------------

        sendMessage(mesRead.serialize());
    }
    // Дочитали память до конца
    else {
//        qInfo() << "[MCFOS] DDR has read all, total bytes:" << recDDR.size();

        // Конвертация и запись результата в файл для дальнейшей обработки в MatLab:
        //--------------------------------------------------------------------
//        qInfo() << "recDDR =" << Qt::endl << recDDR.toHex(' ').toUpper();
//        qInfo() << Qt::endl;

        // Конвертация массива QByteArray в массив 32-разрядых знаковых целых чисел
        QVector<qint32> vecResult;
        for (int i = 0; i < recDDR.size(); i += 4) {
            quint32 word = 0x000000FF & static_cast<quint32>(recDDR[i]);
            word |= 0x0000FF00 & (static_cast<quint32>(recDDR[i + 1]) << 8);
            word |= 0x00FF0000 & (static_cast<quint32>(recDDR[i + 2]) << 16);
            word |= 0xFF000000 & (static_cast<quint32>(recDDR[i + 3]) << 24);
            vecResult.append(static_cast<qint32>(word));
        }
//        qInfo() << "vecResult =" << Qt::endl << vecResult;
//        qInfo() << Qt::endl;

        // Запись массива чисел в текстовый файл для удобства загрузки в MatLab
        QFile *file = new QFile("C:\\Projects\\_Projects_Harrier\\Projects_Xilinx\\MCFOS\\resultFromDDR.txt");
        if (file->open(QIODevice::WriteOnly)) {
            QTextStream out(file);
            for (int i = 0; i < vecResult.size(); i++) {
                out << QString("%1, ").arg(vecResult[i]);
            }
            qInfo() << "[MCFOS] File has wrote by data from DDR";
        } else {
            qInfo() << "[MCFOS] ERROR: cannot open file for write data from DDR";
        }
        file->close();
        //--------------------------------------------------------------------
    }
}

void UdpHandler::processIncomingDatagram(const QByteArray& data,
                                    const QHostAddress& sender,
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

void UdpHandler::sendCustomPacket(const QByteArray& payload,
                                  const QHostAddress& addr,
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
