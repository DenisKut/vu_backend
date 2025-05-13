#include "EthHandler.h"
#include "pch.h"
#include "utils/env_loader/Settings.h"

EthHandler::EthHandler(QObject* parent)
    : QObject(parent), m_adapterOpened(false)
{
#ifdef _WIN32
    pcapAllDevs = nullptr;
    pcapTx = pcapRx = nullptr;
#else
    QString interfaceName = findWiredInterface();
    if (!interfaceName.isEmpty()) {
#ifdef QT_DEBUG
        qDebug() << "Проводной интерфейс:" << interfaceName;
#endif
        strncpy(ifName, interfaceName.toStdString().c_str(), IFNAMSIZ-1);
    } else {
#ifdef QT_DEBUG
        qDebug() << "Интерфейс не найден";
#endif
        LOG_INTERNAL(CLASS_NAME, "MAC SOCKET", "сетевой интерфейс не найден");

        strcpy(ifName, appSettings().value("iface_info/eth_interface_name").toString().toUtf8().constData());
    }
    qDebug() << "interfaceName =" << interfaceName;
#endif
}

EthHandler::~EthHandler() {
    disconnectAdapter();
#ifdef _WIN32
    if (pcapAllDevs) pcap_freealldevs(pcapAllDevs);
#else
    if (ethSocketID != -1) close(ethSocketID);
#endif
}

void EthHandler::initialize() {
    connectAdapter();
}

void EthHandler::start() {
    connectAdapter();
}

void EthHandler::connectAdapter() {
    if (m_adapterOpened) {
        qDebug() << "Ethernet adapter already opened";
        return;
    }
#ifdef _WIN32
    pcap_if_t *d;
    char errbuf[PCAP_ERRBUF_SIZE];

    QStringList strAdapterNames;            // Список имён подключённых адаптеров
    int numAdapters = 0;
    if (pcap_findalldevs_ex(const_cast<char*>(PCAP_SRC_IF_STRING), nullptr, &pcapAllDevs, errbuf) == -1)
    {
#ifdef QT_DEBUG
        qDebug() << "Not found Ethernet adapters";
#endif
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
#ifdef QT_DEBUG
        qDebug() << "Failed to open pcapTx";
#endif
        LOG(InternalMessageLog(CLASS_NAME, "MAC SOCKET","WINPCAP ERROR; EthHandler::connectAdapter() - Не получилось открыть pcapTx"));
        // Free the device list
        return;
    }
    else
    {
//        emit adapterUpdatePM(pcapTx);  // Обновить указатель на адаптер для менеджера пакетов
//        emit adapterConnected();    // Адаптер подключен
#ifdef QT_DEBUG
        qDebug() << "Open pcapTx";
#endif
        LOG(InternalMessageLog(CLASS_NAME, "MAC SOCKET", "WINPCAP; pcapTx открыт"));
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
#ifdef QT_DEBUG
        qDebug() << "Failed to open pcapRx";
#endif
        LOG(InternalMessageLog(CLASS_NAME, "MAC SOCKET","WINPCAP ERROR; EthHandler::connectAdapter() - Не получилось открыть pcapTx"));
        // Free the device list
        return;
    }
    else
    {
//        emit adapterConnected();    // Адаптер подключен
#ifdef QT_DEBUG
        qDebug() << "Open pcapRx";
#endif
        LOG(InternalMessageLog(CLASS_NAME, "MAC SOCKET","WINPCAP; pcapTx открыт"));
        // Logger::instance().writeText("EthHandler::connectAdapter() - Успешно открыт pcapTx");
    }

    pcap_setbuff(pcapRx, 160*1024*1024);
    pcap_setnonblock(pcapRx, 1, errbuf); // 1 - неблокирующий режим
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
#ifdef QT_DEBUG
    qDebug() << QString("Opened Ethernet adapter: %1").arg(strAdapterNames.at(0));
#endif
    LOG(InternalMessageLog(CLASS_NAME, "MAC SOCKET","WINPCAP; " +
    QString("Подключен Ethernet адаптер: %1").arg(strAdapterNames.at(0))));

    m_adapterOpened = true;
#else
    if (ethSocketID != -1) {
        qDebug() << "Ethernet socket already opened";
        m_adapterOpened = true;
        return;
    }
    
    ethSocketID = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (ethSocketID == -1) {
        perror("socket");
        LOG_INTERNAL(CLASS_NAME, "MAC SOCKET", "Ошибка создания raw-сокета");
        return;
    }
    struct ifreq ifopts;
    strncpy(ifopts.ifr_name, ifName, IFNAMSIZ-1);
    ioctl(ethSocketID, SIOCGIFFLAGS, &ifopts);
    ifopts.ifr_flags |= IFF_PROMISC;
    ioctl(ethSocketID, SIOCSIFFLAGS, &ifopts);
    if (setsockopt(ethSocketID, SOL_SOCKET, SO_BINDTODEVICE, ifName, IFNAMSIZ-1) == -1) {
        perror("SO_BINDTODEVICE");
        LOG(InternalMessageLog(CLASS_NAME, "MAC SOCKET", 
        "ОШИБКА : EthHandler::connectAdapter() - SO_BINDTODEVICE "));
        close(ethSocketID);
        ethSocketID = -1;
        return;
    }

    qDebug() << "Starting capture on interface" << ifName << "...\n";
    LOG(InternalMessageLog(CLASS_NAME, "MAC SOCKET", QString("Подключен Ethernet адаптер: %1").arg(ifName)));

    // Создание SocketNotifier
    m_notifier = new QSocketNotifier(ethSocketID, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &EthHandler::checkReceived);
#endif
    m_adapterOpened = true;
}

void EthHandler::disconnectAdapter() {
    if (!m_adapterOpened) return;
#ifdef _WIN32
    if (pcapTx) pcap_close(pcapTx);
    if (pcapRx) pcap_close(pcapRx);
    pcapTx = nullptr;
    pcapRx = nullptr;
#else
    if (ethSocketID != -1) close(ethSocketID);
    ethSocketID = -1;
#endif
    m_adapterOpened = false;
}

bool EthHandler::sendMessage(const QByteArray &data) {
#ifdef _WIN32
    if (pcapTx && pcap_sendpacket(pcapTx, reinterpret_cast<const u_char*>(data.data()), data.size()) == 0)
        return true;
#ifdef QT_DEBUG
    qDebug("Ошибка при отправке Ethernet-пакета");
#endif
    LOG(InternalMessageLog(CLASS_NAME, "MAC SOCKET",
            "WIN32; Ошибка при отправке Ethernet-пакета"));
    return false;
#else
    if (ethSocketID == -1) {
            qDebug() << "Socket not initialized";
            return false;
        }

        // Структура для указания интерфейса и адреса
        struct sockaddr_ll socket_addr;
        memset(&socket_addr, 0, sizeof(socket_addr));

        // Получить индекс интерфейса
        socket_addr.sll_family = AF_PACKET;
        socket_addr.sll_ifindex = if_nametoindex(ifName);
        socket_addr.sll_halen = ETH_ALEN;

        // MAC-адрес получателя (если нужен конкретный)
        // memcpy(socket_addr.sll_addr, dest_mac, ETH_ALEN);

        // Отправка пакета
        ssize_t sent = sendto(
            ethSocketID,
            data.constData(),
            data.size(),
            0,
            (struct sockaddr*)&socket_addr,
            sizeof(socket_addr)
        );

        if (sent == -1) {
            perror("sendto");
            return false;
        }

        return true;
#endif
}

void EthHandler::checkReceived() {
#ifdef _WIN32
    struct pcap_pkthdr *header;
    const unsigned char *data;
    int res = 0;
    if (pcapRx) {
        while ((res = pcap_next_ex(pcapRx, &header, &data)) == 1) {
            QByteArray msg(reinterpret_cast<const char*>(data), header->len);
            LOG(ProtocolMessageLog(CLASS_NAME, Protocol::RAW_MAC, msg, QString("Длинна принятого пакета: %1").arg(header->len)));
            emit messageReceived(msg);
            // QCoreApplication::processEvents(); 	///< @brief Лучше убрать на Windows
													///< может быть даже вреден, если EthHandler не находится в основном потоке
        }
    }
#else
    if (ethSocketID == -1) return;
    const size_t buf_size = appSettings().value("iface_info/eth_socket_buf_size").toULongLong();
    char buf[buf_size];

    // Чтение данных в статический буфер
    ssize_t length = recvfrom(ethSocketID, buf, buf_size, MSG_DONTWAIT, NULL, NULL);
    while(length > 0) {
        // Создаем QByteArray из полученных данных
        QByteArray packet(buf, length);
        emit messageReceived(packet);
        QCoreApplication::processEvents();

        // Читаем следующий пакет
        length = recvfrom(ethSocketID, buf, buf_size, MSG_DONTWAIT, NULL, NULL);
    }
#endif
}

#ifndef _WIN32
bool EthHandler::isEthernetInterface(const QNetworkInterface &interface) {
    QFile typeFile(QString("/sys/class/net/%1/type").arg(interface.name()));
    if (!typeFile.open(QIODevice::ReadOnly)) return false;
    QByteArray data = typeFile.readAll().trimmed();
    bool ok;
    int type = data.toInt(&ok);
    return ok && type == 1;
}

QString EthHandler::findWiredInterface() {
    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces()) {
        if (interface.flags() & QNetworkInterface::IsLoopBack) continue;
        if (!(interface.flags() & QNetworkInterface::IsUp)) continue;
        if (isEthernetInterface(interface)) return interface.name();
    }
    return QString();
}
#endif
