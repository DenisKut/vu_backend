#include "EthHandler.h"
#include <qfile.h>
#include <qfile.h>
#include "pch.h"
#include "utils/env_loader/Settings.h"
// Конструктор обработчика принятых MAC пакетов:
EthHandler::EthHandler(TimeProvider* timeProvider, QObject* parent)
    : QObject(parent), m_timeProvider(timeProvider) 
{
#ifdef _WIN32
    pcapAllDevs = nullptr;
    pcapTx = pcapRx = nullptr;
#else
    // Определение имени проводного интерфейса Ethernet:
//    strcpy(ifName, "enp44s0");
    QString interfaceName = findWiredInterface();
    if (!interfaceName.isEmpty()) {
        qDebug() << "Проводной интерфейс:" << interfaceName;
        strcpy(ifName, interfaceName.toStdString().c_str());
    } else {
        qDebug() << "Интерфейс не найден";
        LOG_INTERNAL(typeid(this).name(), "MAC SOCKET", "сетевой интерфейс не найден");

        strcpy(ifName, appSettings().value("iface_info/eth_interface_name").toString().toUtf8().constData());
    }
#endif
}

EthHandler::~EthHandler() {
    disconnectAdapter();
#ifdef _WIN32
    if (pcapAllDevs) pcap_freealldevs(pcapAllDevs);
#else
    close(ethSocketID);
#endif
}

#ifndef _WIN32
bool EthHandler::isEthernetInterface(const QNetworkInterface &interface)
{
    // Проверяем тип интерфейса через sysfs
    QFile typeFile(QString("/sys/class/net/%1/type").arg(interface.name()));
    if (!typeFile.open(QIODevice::ReadOnly)) return false;

    QByteArray data = typeFile.readAll().trimmed();
    bool ok;
    int type = data.toInt(&ok);

    // 1 соответствует Ethernet (ARPHRD_ETHER)
    return ok && type == 1;
}

// Поиск проводного интерфейса Ethernet:
QString EthHandler::findWiredInterface() {
    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces()) {
        // Пропускаем loopback и выключенные интерфейсы
        if (interface.flags() & QNetworkInterface::IsLoopBack) continue;
        if (!(interface.flags() & QNetworkInterface::IsUp)) continue;

        if (isEthernetInterface(interface)) {
            return interface.name();
        }
    }
    return QString();
}
#endif

quint64 EthHandler::getTimestamp()
{
    auto now_ns = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::time_point_cast<std::chrono::nanoseconds>(now_ns);
    quint64 time_ns = ns.time_since_epoch().count();
    return time_ns;
}

// Подключиться к адаптеру Fast Ethernet:
void EthHandler::connectAdapter()
{
#ifdef _WIN32
    pcap_if_t *d;
    char errbuf[PCAP_ERRBUF_SIZE];

    QStringList strAdapterNames;            // Список имён подключённых адаптеров
    int numAdapters = 0;
    if (pcap_findalldevs_ex(const_cast<char*>(PCAP_SRC_IF_STRING), nullptr, &pcapAllDevs, errbuf) == -1)
    {
        qDebug() << "Not found Ethernet adapters";
       // Logger::instance().writeText("ОШИБКА: EthHandler::connectAdapter() - Не найден сетевой адаптер");
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
        strAdapterNames.append(ds);
        numAdapters++;
    }

    // Выбрать адаптер, если он есть:
    int inum = 0;
    for (inum = 0; inum < numAdapters; inum++)
    {
        if (strAdapterNames[inum].contains("Microsoft"))
        {
            continue;
        }
        else
        {
            break;
        }
    }

    // Переход к выбранному адаптеру:
    int i = 0;
    //------------------------------------------------------------------
    for (d = pcapAllDevs, i = 0; i < inum; d = d->next, i++);
    //------------------------------------------------------------------

    // Подключить передатчик к выбранному адаптеру:
    //------------------------------------------------------------------
    if ( (pcapTx = pcap_open(d->name,     // name of the device
                               2048,     // portion of the packet to capture.
                                            // 65536 guarantees that the whole packet will be captured on all the link layers
                               PCAP_OPENFLAG_PROMISCUOUS| PCAP_OPENFLAG_MAX_RESPONSIVENESS|PCAP_OPENFLAG_NOCAPTURE_LOCAL,    // promiscuous mode
//                               PCAP_OPENFLAG_PROMISCUOUS,    // promiscuous mode
                               10,          // read timeout
                               nullptr,     // authentication on the remote machine
                               errbuf       // error buffer
                               ) ) == nullptr)
    {
        qDebug() << "Failed to open pcapTx";
        LOG(InternalMessageLog(typeid(this).name(), "MAC SOCKET"."WINPCAP ERROR; EthHandler::connectAdapter() - Не получилось открыть pcapTx"));
        // Free the device list
        return;
    }
    else
    {
//        emit adapterUpdatePM(pcapTx);  // Обновить указатель на адаптер для менеджера пакетов
//        emit adapterConnected();    // Адаптер подключен
        qDebug() << "Open pcapTx";
        LOG(InternalMessageLog(typeid(this).name(), "MAC SOCKET"."WINPCAP; pcapTx открыт"));
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
        LOG(InternalMessageLog(typeid(this).name(), "MAC SOCKET"."WINPCAP ERROR; EthHandler::connectAdapter() - Не получилось открыть pcapTx"));
        // Free the device list
        return;
    }
    else
    {
//        emit adapterConnected();    // Адаптер подключен
        qDebug() << "Open pcapRx";
        LOG(InternalMessageLog(typeid(this).name(), "MAC SOCKET"."WINPCAP; pcapTx открыт"));
        // Logger::instance().writeText("EthHandler::connectAdapter() - Успешно открыт pcapTx");
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
    LOG(InternalMessageLog(typeid(this).name(), "MAC SOCKET"."WINPCAP; " + 
    QString("Подключен Ethernet адаптер: %1").arg(strAdapterNames.at(0))));
#else
    // ETH_P_ALL                - принимать пакеты с любым ether type
    // ETH_PROTO_ETHER_TYPE()   - принимать пакеты только с ether type = 0x2020
    uint64_t macDst = AppConfig::instance().ETH_PROTO_MAC_DST();
    uint64_t macSrc = AppConfig::instance().ETH_PROTO_MAC_SRC();
    uint16_t etherType = AppConfig::instance().ETH_PROTO_ETHER_TYPE();
    qDebug() << "macDst =" << macDst;
    qDebug() << "macSrc =" << macSrc;
    qDebug() << "etherType =" << etherType;
    //if ((ethSocketID = socket(PF_PACKET, SOCK_RAW, htons(etherType))) == -1) {
    if ((ethSocketID = socket(PF_PACKET, SOCK_RAW, htons(0x2020))) == -1) {
        perror("socket");
        LOG(InternalMessageLog(typeid(this).name(), "MAC SOCKET", "ОШИБКА: EthHandler::connectAdapter() - socket"));
    }

    struct ifreq ifopts;
    strncpy(ifopts.ifr_name, ifName, IFNAMSIZ-1);
    ioctl(ethSocketID, SIOCGIFFLAGS, &ifopts);
    ifopts.ifr_flags |= IFF_PROMISC;
    ioctl(ethSocketID, SIOCSIFFLAGS, &ifopts);

    if (setsockopt(ethSocketID, SOL_SOCKET, SO_BINDTODEVICE, ifName, IFNAMSIZ-1) == -1) {
        perror("SO_BINDTODEVICE");
        LOG(InternalMessageLog(typeid(this).name(), "MAC SOCKET", 
        "ОШИБКА : EthHandler::connectAdapter() - SO_BINDTODEVICE "));
        close(ethSocketID);
    }

    qDebug() << "Starting capture on interface" << ifName << "...\n";
    LOG(InternalMessageLog(typeid(this).name(), "MAC SOCKET", QString("Подключен Ethernet адаптер: %1").arg(ifName)));

    // Создание SocketNotifier
    m_notifier = new QSocketNotifier(ethSocketID, QSocketNotifier::Read, this);

    // Подключение сигнала к слоту
    connect(m_notifier, &QSocketNotifier::activated,
            this, &EthHandler::checkReceived);
#endif
}

// Отключиться от адаптера Fast Ethernet:
void EthHandler::disconnectAdapter()
{
#ifdef _WIN32
    if (pcapTx != nullptr) pcap_close(pcapTx);
    if (pcapRx != nullptr) pcap_close(pcapRx);
    pcapTx = nullptr;
    pcapRx = nullptr;
#else
    close(ethSocketID);
#endif
    // Logger::instance().writeText("Ethernet адаптер отключен");
}

/// Отправить сообщение
bool EthHandler::sendMessage(const QByteArray &data)
{
#ifdef _WIN32
    if (pcap_sendpacket(pcapTx, reinterpret_cast<const u_char*>(data.data()), data.size()) != 0)
    {
        qDebug("Ошибка при отправке Ethernet-пакета");
        LOG(InternalMessageLog(typeid(this).name(), "MAC SOCKET",
                               "WIN32; Ошибка при отправке Ethernet-пакета"));
        return false;
    }
    return true;
#else
    return false;
#endif
}

void EthHandler::start()
{
    connectAdapter();
}

// Проверка сокета на наличие принятых Ethernet-пакетов:
void EthHandler::checkReceived()
{
#ifdef _WIN32
    struct pcap_pkthdr *header;
    const unsigned char *data;
    int res = 0;

    // Проверка приёмника на наличие пакета:
    if (pcapRx != nullptr)
    {
        while ((res = pcap_next_ex(pcapRx, &header, &data)) == 1) // Пока приёмник не пустой (можно if вместо while, но тогда будет прочитан 1 пакет)
        {
            // Общие параметры пакета:
            long long time = static_cast<long long>(header->ts.tv_sec)*1000000 + static_cast<long long>(header->ts.tv_usec);
            int length = static_cast<int>(header->len);

            // Запись в лог метки времени и длины пакета:
//            qDebug() << QString("%1  -  %2").arg(time).arg(length);
            //Logger::instance().writeBinary("Принят Ethernet пакет:", data, length);
            
            // Отправка данных в NetworkManager для разбора:
            QByteArray msg = QByteArray(reinterpret_cast<const char*>(data), length);
            LOG(ProtocolMessageLog(typeid(this).name(), Protocol::RAW_MAC, msg, QString("Длинна принятого пакета: %1").arg(length)));
            emit messageReceived(msg);
        }
    }
#else
    // Чтение пакета из интерфейса:
    //---------------------------------------------------------------------

    const size_t buf_size = appSettings().value("iface_info/eth_socket_buf_size").toULongLong();

    uint8_t *buf = static_cast<uint8_t *>(malloc(buf_size * sizeof(uint8_t)));

    // Проверка приёмника на наличие пакета:
    ssize_t length = 0;
    while((length = recvfrom(ethSocketID, buf, buf_size, 0, NULL, NULL)) > 0)
    {
        // Запись в лог метки времени и длины пакета:
//            qDebug() << QString("%1  -  %2").arg(time).arg(length);
        //Logger::instance().writeBinary("Принят Ethernet пакет:", buf, length);

        // Отправка данных в NetworkManager для разбора:
        QByteArray msg = QByteArray(reinterpret_cast<const char*>(buf), length);
        LOG(ProtocolMessageLog(typeid(this).name(), Protocol::RAW_MAC, msg, QString("Длинна принятого пакета: %1").arg(length)));
        emit messageReceived(msg);
    }
    free(buf);
    //---------------------------------------------------------------------
#endif
}
