#include "MacHandler.h"

// Конструктор обработчика принятых MAC пакетов:
MacHandler::MacHandler(TimeProvider* timeProvider, QObject* parent)
    : QObject(parent), m_timeProvider(timeProvider) 
{
    pcapAllDevs = nullptr;
    pcapTx = pcapRx = nullptr;
}

MacHandler::~MacHandler() {
    disconnectAdapter();
    if (pcapAllDevs) pcap_freealldevs(pcapAllDevs);
}

// Возвращает список имён существующих адаптеров Fast Ethernet:
QVector<QString> MacHandler::getNamesAdapters()
{
    pcap_if_t *d;
    QVector<QString> out;
    char errbuf[PCAP_ERRBUF_SIZE];
    if (pcap_findalldevs_ex(const_cast<char*>(PCAP_SRC_IF_STRING), nullptr, &pcapAllDevs, errbuf) == -1)
    {
        qDebug() << "Not found Ethernet adapters";
    }
    for(d = pcapAllDevs; d; d = d->next)
    {
        QString ds = ":)";
        if (d->description)
        {
            ds = QString("%1").arg(d->description);
        }
        else
        {
            ds = QString("%1").arg(d->name);
        }
        out.append(ds);
    }
    return out;
}

// Подключиться к адаптеру Fast Ethernet под номером inum в списке адаптеров:
void MacHandler::connectAdapter(int inum)
{
    pcap_if_t *d;
    char errbuf[PCAP_ERRBUF_SIZE];

    // Переход к выбранному адаптеру:
    //------------------------------------------------------------------
    int i = 0;
    for (d = pcapAllDevs, i = 0; i < inum; d = d->next, i++);
    //------------------------------------------------------------------

    // Подключить передатчик к выбранному адаптеру:
    //------------------------------------------------------------------
    if ( (pcapTx = pcap_open(d->name,     // name of the device
                               65536,     // portion of the packet to capture.
                                            // 65536 guarantees that the whole packet will be captured on all the link layers
                               PCAP_OPENFLAG_PROMISCUOUS| PCAP_OPENFLAG_MAX_RESPONSIVENESS|PCAP_OPENFLAG_NOCAPTURE_LOCAL,    // promiscuous mode
//                               PCAP_OPENFLAG_PROMISCUOUS,    // promiscuous mode
                               10,          // read timeout
                               nullptr,     // authentication on the remote machine
                               errbuf       // error buffer
                               ) ) == nullptr)
    {
        qDebug() << "Failed to open pcapTx";
        // Free the device list
        return;
    }
    else
    {
//        emit adapterUpdatePM(pcapTx);  // Обновить указатель на адаптер для менеджера пакетов
//        emit adapterConnected();    // Адаптер подключен
        qDebug() << "Open pcapTx";
    }

    pcap_setbuff(pcapTx, 160*1024*1024);
    pcap_setnonblock(pcapTx, 0, errbuf);
    //------------------------------------------------------------------

    // Подключить приёмник к выбранному адаптеру:
    //------------------------------------------------------------------
    if ( (pcapRx = pcap_open(d->name,     // name of the device
                               65536,     // portion of the packet to capture.
                                            // 65536 guarantees that the whole packet will be captured on all the link layers
                               PCAP_OPENFLAG_PROMISCUOUS| PCAP_OPENFLAG_MAX_RESPONSIVENESS|PCAP_OPENFLAG_NOCAPTURE_LOCAL,    // promiscuous mode
//                               PCAP_OPENFLAG_PROMISCUOUS,    // promiscuous mode
                               10,          // read timeout
                               nullptr,     // authentication on the remote machine
                               errbuf       // error buffer
                               ) ) == nullptr)
    {
        qDebug() << "Failed to open pcapRx";
        // Free the device list
        return;
    }
    else
    {
//        emit adapterConnected();    // Адаптер подключен
        qDebug() << "Open pcapRx";
    }

    pcap_setbuff(pcapRx, 160*1024*1024);
    pcap_setnonblock(pcapRx, 0, errbuf);
    //------------------------------------------------------------------

    // Кладём имя адаптера в список подключенных адаптеров:
    QString ds = ":)";
    if (d->description)
    {
        ds = QString("%1").arg(d->description);
//        ds.sprintf("%d. %s", ++i, d->description);
    }
    else
    {
        ds = QString("%1").arg(d->name);
//        ds.sprintf("%d. %s", ++i, d->name);
    }
    strAdapterNames.append(ds);

    qDebug() << QString("Opened Ethernet adapter: %1").arg(strAdapterNames.at(0));
}

// Отключиться от адаптера Fast Ethernet:
void MacHandler::disconnectAdapter()
{
    if (pcapTx != nullptr) pcap_close(pcapTx);
    if (pcapRx != nullptr) pcap_close(pcapRx);
//    emit adapterUpdatePM(nullptr);  // Обновить указатель на адаптер для менеджера пакетов
    pcapTx = nullptr;
    pcapRx = nullptr;
//    emit adapterDisconnected();
}

// Обработчик принятых MAC пакетов:
void MacHandler::messageProccess(const Message &msg)
{

}

void MacHandler::start()
{
    QVector<QString> vecAdapterNames;
    vecAdapterNames.append(getNamesAdapters());
    qDebug() << vecAdapterNames;

//-------------------------DEBUG----------------------------
    // Выбрать адаптер, если он есть:
    int i = 0;
    for (i = 0; i < vecAdapterNames.size(); i++)
    {
        if (vecAdapterNames[i].contains("Microsoft"))
        {
            continue;
        }
        else
        {
            break;
        }
    }
//----------------------------------------------------------

    connectAdapter(i);
}

// Проверка сокета на наличие принятых MAC пакетов:
void MacHandler::checkReceived()
{
    struct pcap_pkthdr *header;
    const u_char *data;
    int res = 0;

    // Проверка приёмника на наличие пакета:
    if (pcapRx != nullptr)
    {
        while ((res = pcap_next_ex(pcapRx, &header, &data)) == 1) // Пока приёмник не пустой (можно if вместо while, но тогда будет прочитан 1 пакет)
        {
            // Общие параметры пакета:
            long long time = static_cast<long long>(header->ts.tv_sec)*1000000 + static_cast<long long>(header->ts.tv_usec);
            int length = static_cast<int>(header->len);

//            vecRecTime.append(time);                      // Метка времени пакета
//            vecRecData.append(QByteArray(data, length));  // Пакет

            // Вывод метки времени и длины пакета:
            qDebug() << QString("%1  -  %2").arg(time).arg(length);
        }
    }

    // Перезапуск таймера вызова обработчика принятых MAC пакетов:
    macTimerReceiveRestart();
}
