#include "EthHandler.h"


// Конструктор обработчика принятых MAC пакетов:
EthHandler::EthHandler(TimeProvider* timeProvider, QObject* parent)
    : QObject(parent), m_timeProvider(timeProvider) 
{
#ifdef _WIN32
    pcapAllDevs = nullptr;
    pcapTx = pcapRx = nullptr;
#else
    strcpy(ifName, AppConfig::instance().ETH_INTERFACE_NAME().c_str());
#endif
}

EthHandler::~EthHandler() {
    disconnectAdapter();
#ifdef _WIN32
    if (pcapAllDevs) pcap_freealldevs(pcapAllDevs);
#else
    close(macSocketID);
//    this->consts->
#endif
}


#ifndef _WIN32
std::string EthHandler::getTimestampStr()
{
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::microseconds>(
                now.time_since_epoch()) % 1000000;
    auto timer = std::chrono::system_clock::to_time_t(now);
    struct tm tm = *std::localtime(&timer);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S") << "."
        << std::setfill('0') << std::setw(6) << ms.count();
    return oss.str();
}
#endif

quint64 EthHandler::getTimestamp()
{
    auto now_ns = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::time_point_cast<std::chrono::nanoseconds>(now_ns);
    quint64 time_ns = ns.time_since_epoch().count();
    return time_ns;
}

#ifndef _WIN32
void EthHandler::print_mac(const uint8_t *mac)
{
    printf("%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void EthHandler::print_hex_dump(const uint8_t *buf, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (i % 16 == 0) {
            printf("\n\t0x%04x:  ", (unsigned int)i);
        }
        printf("%02x", buf[i]); // Убрали пробел между байтами
        if (i % 2 == 1) printf(" "); // Пробел только после каждой пары байтов
    }
    printf("\n");
}
#endif

// Подключиться к адаптеру Fast Ethernet под номером inum в списке адаптеров:
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
        Logger::instance().writeText("ОШИБКА: EthHandler::connectAdapter() - Не найден сетевой адаптер");
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
        Logger::instance().writeText("ОШИБКА: EthHandler::connectAdapter() - Не получилось открыть pcapTx");
        // Free the device list
        return;
    }
    else
    {
//        emit adapterUpdatePM(pcapTx);  // Обновить указатель на адаптер для менеджера пакетов
//        emit adapterConnected();    // Адаптер подключен
        qDebug() << "Open pcapTx";
        Logger::instance().writeText("EthHandler::connectAdapter() - Успешно открыт pcapTx");
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
        Logger::instance().writeText("ОШИБКА: EthHandler::connectAdapter() - Не получилось открыть pcapTx");
        // Free the device list
        return;
    }
    else
    {
//        emit adapterConnected();    // Адаптер подключен
        qDebug() << "Open pcapRx";
        Logger::instance().writeText("EthHandler::connectAdapter() - Успешно открыт pcapTx");
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
    Logger::instance().writeText(QString("Подключен Ethernet адаптер: %1").arg(strAdapterNames.at(0)));
#else
    Q_UNUSED(inum);

    // ETH_P_ALL                - принимать пакеты с любым ether type
    // ETH_SOCKET_ETHER_TYPE    - принимать пакеты только с ether type = 0x2020
    if ((macSocketID = socket(PF_PACKET, SOCK_RAW, htons(ETH_SOCKET_ETHER_TYPE))) == -1) {
        perror("socket");
        Logger::instance().writeText("ОШИБКА: EthHandler::connectAdapter() - socket");
    }

    struct ifreq ifopts;
    strncpy(ifopts.ifr_name, ifName, IFNAMSIZ-1);
    ioctl(macSocketID, SIOCGIFFLAGS, &ifopts);
    ifopts.ifr_flags |= IFF_PROMISC;
    ioctl(macSocketID, SIOCSIFFLAGS, &ifopts);

    if (setsockopt(macSocketID, SOL_SOCKET, SO_BINDTODEVICE, ifName, IFNAMSIZ-1) == -1) {
        perror("SO_BINDTODEVICE");
        Logger::instance().writeText("ОШИБКА: EthHandler::connectAdapter() - SO_BINDTODEVICE");
        close(macSocketID);
    }

    qDebug() << "Starting capture on interface" << ifName << "...\n";
    Logger::instance().writeText(QString("Подключен Ethernet адаптер: %1").arg(ifName));

    // Создание SocketNotifier
    m_notifier = new QSocketNotifier(macSocketID, QSocketNotifier::Read, this);

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
    close(macSocketID);
#endif
    Logger::instance().writeText("Ethernet адаптер отключен");
}

void EthHandler::start()
{
    connectAdapter();
}

// Проверка сокета на наличие принятых MAC пакетов:
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

            // Вывод метки времени и длины пакета:
            qDebug() << QString("%1  -  %2").arg(time).arg(length);
            //Logger::instance().writeText(QString("%1  -  %2").arg(time).arg(length));
            Logger::instance().writeBinary("Принят Ethernet пакет:", data, length);
        }
    }
#else
    // Чтение пакета из интерфейса:
    //---------------------------------------------------------------------
    const buf_size = AppConfig::instance().MAC_SOCKET_BUF_SIZE();
    uint8_t *buf = static_cast<uint8_t*>(malloc(buf_size * sizeof(uint8_t)));
    ssize_t numbytes = recvfrom(macSocketID, buf, buf_size, 0, NULL, NULL);
    if (numbytes > 0)
    {
        Message *msg = new Message();
        msg->timestamp = getTimestamp();
        msg->payload.append(QByteArray(reinterpret_cast<const char*>(buf), numbytes));

        struct ethhdr* eth = (struct ethhdr*)buf;

        // Вывод данных пакета:
        //---------------------------------------------------------------------
        printf("%s ", getTimestampStr().c_str());

        print_mac(eth->h_source);
        printf(" > ");
        print_mac(eth->h_dest);
        printf(", ");

        uint16_t ethertype = ntohs(eth->h_proto);
        switch (ethertype) {
            case ETH_P_IP:  printf("ethertype IPv4 (0x%04x), ", ethertype); break;
            case ETH_P_ARP: printf("ethertype ARP (0x%04x), ", ethertype); break;
            default:        printf("ethertype Unknown (0x%04x), ", ethertype); break;
        }

        printf("length %zd:", numbytes);

        print_hex_dump(buf, numbytes);
        printf("\n");
        //---------------------------------------------------------------------

        emit messageReceived(*msg);
    }
    free(buf);
    //---------------------------------------------------------------------
#endif
}
