#include "EthHandler.h"
#include <QDebug>
#include "core/logger/Logger.h"
#include "utils/AppConfig.h"

EthHandler::EthHandler(TimeProvider* timeProvider, QObject* parent)
    : QObject(parent), m_timeProvider(timeProvider), m_adapterOpened(false)
{
#ifdef _WIN32
    pcapAllDevs = nullptr;
    pcapTx = pcapRx = nullptr;
#else
    QString interfaceName = findWiredInterface();
    if (!interfaceName.isEmpty()) {
        strncpy(ifName, interfaceName.toStdString().c_str(), IFNAMSIZ-1);
    } else {
        strncpy(ifName, AppConfig::instance().ETH_INTERFACE_NAME().c_str(), IFNAMSIZ-1);
    }
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
    if (ethSocketID != -1) {
        qDebug() << "Ethernet socket already opened";
        m_adapterOpened = true;
        return;
    }
    if ((ethSocketID = socket(PF_PACKET, SOCK_RAW, htons(0x2020))) == -1) {
        perror("socket");
        Logger::instance().writeText("ОШИБКА: EthHandler::connectAdapter() - socket");
        return;
    }
    struct ifreq ifopts;
    strncpy(ifopts.ifr_name, ifName, IFNAMSIZ-1);
    ioctl(ethSocketID, SIOCGIFFLAGS, &ifopts);
    ifopts.ifr_flags |= IFF_PROMISC;
    ioctl(ethSocketID, SIOCSIFFLAGS, &ifopts);
    if (setsockopt(ethSocketID, SOL_SOCKET, SO_BINDTODEVICE, ifName, IFNAMSIZ-1) == -1) {
        perror("SO_BINDTODEVICE");
        Logger::instance().writeText("ОШИБКА: EthHandler::connectAdapter() - SO_BINDTODEVICE");
        close(ethSocketID);
        ethSocketID = -1;
        return;
    }
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
    Logger::instance().writeText("Ethernet адаптер отключен");
}

bool EthHandler::sendMessage(const QByteArray &data) {
#ifdef _WIN32
    if (pcapTx && pcap_sendpacket(pcapTx, reinterpret_cast<const u_char*>(data.data()), data.size()) == 0)
        return true;
    qDebug("Ошибка при отправке Ethernet-пакета");
    return false;
#else
    if (ethSocketID == -1) return false;
    ssize_t sent = send(ethSocketID, data.constData(), data.size(), 0);
    return sent == data.size();
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
            emit messageReceived(msg);
            QCoreApplication::processEvents();
        }
    }
#else
    if (ethSocketID == -1) return;
    const size_t buf_size = AppConfig::instance().ETH_SOCKET_BUF_SIZE();
    QByteArray buf;
    buf.resize(buf_size);
    ssize_t length = 0;
    while((length = recvfrom(ethSocketID, buf.data(), buf_size, 0, NULL, NULL)) > 0) {
        emit messageReceived(buf.left(length));
        QCoreApplication::processEvents();
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