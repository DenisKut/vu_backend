#include "EthHandler.h"
#include "pch.h"
#include "utils/env_loader/Settings.h"
//DS ADD BEG
#ifdef _WIN32
bool EthHandler::setupCustomProtocolFilter(pcap_t* handle, u_int customEtherType) {
    if (!handle) return false;

    struct bpf_program fp;
    char filter_exp[100];

    // Создаем фильтр для кастомного EtherType
    snprintf(filter_exp, sizeof(filter_exp), "ether proto 0x%x", customEtherType);

    // Компилируем фильтр
    if (pcap_compile(handle, &fp, filter_exp, 0, PCAP_NETMASK_UNKNOWN) == -1) {
        return false;
    }

    // Устанавливаем фильтр
    if (pcap_setfilter(handle, &fp) == -1) {
        pcap_freecode(&fp);
        return false;
    }

    pcap_freecode(&fp);
    return true;
}

bool EthHandler::isReceivedPacket(pcap_t *testHandle)
{
    struct pcap_pkthdr *header;
    const u_char *packetData;
    time_t startTime = time(NULL);
    bool packetReceived = false;
    bool breakWh = false;
    while (breakWh == false) { // 1 секунда таймаут
        int res = pcap_next_ex(testHandle, &header, &packetData);

        if (res == 1) {
            packetReceived = true;
            breakWh = true;
            return packetReceived;
        }
        // else if (res == -1) {
        //     break;
        // }
        // Если res == 0 (таймаут), продолжаем ожидание
        if(time(NULL) - startTime < 3)
        {
            return packetReceived;
        }
        QThread::msleep(20);

    }
    return packetReceived;
}

bool EthHandler::adapterCheckTypeReceived(pcap_if_t *d)
{
    char errbuf[PCAP_ERRBUF_SIZE];

    // Пробуем открыть адаптер для проверки поддержки протокола
    pcap_t *testHandle = pcap_open(d->name, 65536, PCAP_OPENFLAG_PROMISCUOUS, 100, nullptr, errbuf);

    if (!testHandle) return false;

    // Проверяем поддержку Ethernet заголовков
    if (pcap_datalink(testHandle) != DLT_EN10MB) {
        pcap_close(testHandle);
        return false;
    }

    // Пробуем установить фильтр для нашего протокола
    if (setupCustomProtocolFilter(testHandle, m_customEtherType)) {

        bool packetReceived = isReceivedPacket(testHandle);
        pcap_close(testHandle);
        return packetReceived;
    }

    pcap_close(testHandle);

    return false;
}

pcap_if_t* EthHandler::findAdapters(pcap_if_t *pcapAllDevs)
{
    pcap_if_t *d;
    for(d = pcapAllDevs; d; d = d->next)
    {
        if(adapterCheckTypeReceived(d))
        {
            return d;
        }

    }
    return nullptr;
}

//16.05 BEG


void EthHandler::startPollingThread() {

    if (m_pollThread) {
        return; // Уже запущен
    }

    if (!m_adapterOpened) {  // ← А вот эта проверка здесь имеет смысл
        qInfo() << "Cannot start polling - adapter not opened";
        return;
    }

    m_polling = true;
    m_pollThread = QThread::create([this]() {
        while (m_polling) {
            this->checkReceived();
            QThread::usleep(500); // 0.5 мс
        }
    });
    m_pollThread->start();

    qDebug() << "Polling thread started";
}

void EthHandler::stopPollingThread() {
    if (!m_pollThread) {
        return;
    }

    m_polling = false;
    m_pollThread->quit();
    m_pollThread->wait();
    delete m_pollThread;
    m_pollThread = nullptr;

    qDebug() << "Polling thread stopped";
}
//16.05 END

#endif
//DS ADD END


//16.05 ADD BEG
void EthHandler::startReconnectTimer() {
    /*
    if (!m_reconnectTimer) {
        m_reconnectTimer = new QTimer(this);
        connect(m_reconnectTimer, &QTimer::timeout, this, &EthHandler::attemptReconnect);
    }
    */
    m_reconnectTimer->start(RECONNECT_INTERVAL_MS);
}

void EthHandler::stopReconnectTimer() {
    /*if (m_reconnectTimer) {
        m_reconnectTimer->stop();
    }*/
    m_reconnectTimer->stop();
    m_reconnectAttempts = 0;
}

void EthHandler::attemptReconnect() {
    /*
    if (m_reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
        qInfo() << "Max reconnect attempts reached, stopping...";
        stopReconnectTimer();
        return;
    }
    */

    m_reconnectAttempts++;
    qInfo() << "Attempting to reconnect, attempt" << m_reconnectAttempts;

    disconnectAdapter();
    QThread::msleep(500); // Даем время на корректное закрытие
    connectAdapter();
}
//16.05 ADD END

EthHandler::EthHandler(QObject* parent)
    : QObject(parent), m_adapterOpened(false)
{
// ADD 16.05 BEG
//#ifdef _WIN32
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(false);
    connect(m_reconnectTimer, &QTimer::timeout, this, &EthHandler::attemptReconnect);
//#endif
// ADD 16.05 END

#ifndef _WIN32

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
#endif
}

void EthHandler::freeRes() {

#ifdef _WIN32
    if (pcapAllDevs) pcap_freealldevs(pcapAllDevs);
#endif

    disconnectAdapter();

}

void EthHandler::initialize() {
    //connectAdapter();
        connectAdapter();

}

void EthHandler::start() {
    connectAdapter();
}

void EthHandler::connectAdapter() {

    if (m_adapterOpened) {

#ifdef QT_DEBUG
        qInfo() << "Ethernet adapter already opened";
#endif
        return;
    }
//ADD 16.05
    stopReconnectTimer();
//END 16.05
#ifdef _WIN32
    // Остановка таймера переподключения при успешном подключении
    //stopReconnectTimer();

    char errbuf[PCAP_ERRBUF_SIZE];

    if (pcap_findalldevs_ex(const_cast<char*>(PCAP_SRC_IF_STRING), nullptr, &pcapAllDevs, errbuf) == -1)
    {

#ifdef QT_DEBUG
        qInfo() << "Not found Ethernet adapters";
#endif

        freeRes();
        startReconnectTimer();
        return;
    }

    pcap_if_t * d = findAdapters(pcapAllDevs);

    if(d == nullptr)
    {
#ifdef QT_DEBUG
        qInfo() << "No suitable adapter found for EtherType 0x" << QString::number(m_customEtherType, 16);
#endif
        freeRes();
        startReconnectTimer();
        return;
    }

#ifdef QT_DEBUG
    qInfo() << "Choosen adapter: " << d->name;
#endif

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
        freeRes();
        startReconnectTimer();
        return;
    }
    else
    {
//        emit adapterUpdatePM(pcapTx);  // Обновить указатель на адаптер для менеджера пакетов
//        emit adapterConnected();    // Адаптер подключен
        qDebug() << "Open pcapTx";
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
        freeRes();
        startReconnectTimer();
        return;
    }
    else
    {
//        emit adapterConnected();    // Адаптер подключен
#ifdef QT_DEBUG
        qDebug() << "Open pcapRx";
#endif
        LOG(InternalMessageLog(CLASS_NAME, "MAC SOCKET","WINPCAP; pcapTx открыт"));
    }

    pcap_setbuff(pcapRx, 160*1024*1024);
    pcap_setnonblock(pcapRx, 1, errbuf); // 1 - неблокирующий режим
    //------------------------------------------------------------------

    // Кладём имя адаптера в список подключенных адаптеров:

    QString ds = "";

    if (d->description)
    {
        ds = d->description;
    }
    else
    {
        ds = d->name;
    }

#ifdef QT_DEBUG
    qDebug() << "Opened Ethernet adapter: " << ds;
#endif
    LOG(InternalMessageLog(CLASS_NAME, "MAC SOCKET","WINPCAP; " + ds));

    if (!setupCustomProtocolFilter(pcapRx, m_customEtherType)) {
        freeRes();
        startReconnectTimer();
        return;
    }

    m_adapterOpened = true;

    m_lastPacketTime = QDateTime::currentDateTime();
    stopReconnectTimer(); // На всякий случай останавливаем таймер
	// Запуск отдельного polling-потока
    startPollingThread();
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
        startReconnectTimer();
        return;
    }

    struct ifreq ifopts;
    /*
    strncpy(ifopts.ifr_name, ifName, IFNAMSIZ-1);
    ioctl(ethSocketID, SIOCGIFFLAGS, &ifopts);
    ifopts.ifr_flags |= IFF_PROMISC;
    ioctl(ethSocketID, SIOCSIFFLAGS, &ifopts);
*/
    strncpy(ifopts.ifr_name, ifName, IFNAMSIZ-1);
    if (ioctl(ethSocketID, SIOCGIFFLAGS, &ifopts) == -1) {
        perror("SIOCGIFFLAGS");
        close(ethSocketID);
        ethSocketID = -1;
        startReconnectTimer();
        return;
    }

    ifopts.ifr_flags |= IFF_PROMISC;
    if (ioctl(ethSocketID, SIOCSIFFLAGS, &ifopts) == -1) {
        perror("SIOCSIFFLAGS");
        close(ethSocketID);
        ethSocketID = -1;
        startReconnectTimer();
        return;
    }

    if (setsockopt(ethSocketID, SOL_SOCKET, SO_BINDTODEVICE, ifName, IFNAMSIZ-1) == -1) {
        perror("SO_BINDTODEVICE");
        LOG(InternalMessageLog(CLASS_NAME, "MAC SOCKET", 
        "ОШИБКА : EthHandler::connectAdapter() - SO_BINDTODEVICE "));
        close(ethSocketID);
        ethSocketID = -1;
        startReconnectTimer();
        return;
    }

    qDebug() << "Starting capture on interface" << ifName << "...\n";
    LOG(InternalMessageLog(CLASS_NAME, "MAC SOCKET", QString("Подключен Ethernet адаптер: %1").arg(ifName)));

    // Создание SocketNotifier
    if (m_notifier) {
        m_notifier->setEnabled(false);  // Отключаем новые события
        disconnect(m_notifier, &QSocketNotifier::activated, this, &EthHandler::checkReceived);
        m_notifier->deleteLater();  // Безопасное удаление
        m_notifier = nullptr;      // Обнуляем указатель
    }
    m_notifier = new QSocketNotifier(ethSocketID, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &EthHandler::checkReceived);

    m_lastPacketTime = QDateTime::currentDateTime();
#endif
    m_adapterOpened = true;
}

void EthHandler::disconnectAdapter() {
    if (!m_adapterOpened) return;
#ifdef _WIN32
    stopPollingThread();
    if (pcapTx) pcap_close(pcapTx);
    if (pcapRx) pcap_close(pcapRx);
    pcapTx = nullptr;
    pcapRx = nullptr;
#else
    if (m_notifier) {
        /*
        delete m_notifier;
        m_notifier = nullptr;
        */
        m_notifier->setEnabled(false);
        disconnect(m_notifier, &QSocketNotifier::activated, this, &EthHandler::checkReceived);
        m_notifier->deleteLater();
        m_notifier = nullptr;

    }
    if (ethSocketID != -1)
        close(ethSocketID);
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
    LOG(InternalMessageLog(CLASS_NAME, "MAC SOCKET","WIN32; Ошибка при отправке Ethernet-пакета"));
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
    //add 16.05 BEG
    bool errorOccurred = false;
    bool timeoutOccurred = false;
    //add 16.05 END
#ifdef _WIN32
    struct pcap_pkthdr *header;
    const unsigned char *data;
    int res = 0;


    if (pcapRx)
    {
        while (res = pcap_next_ex(pcapRx, &header, &data))
        {
            if(res == 1)
            {
                m_lastPacketTime = QDateTime::currentDateTime();

                QByteArray msg(reinterpret_cast<const char*>(data), header->len);
                LOG(ProtocolMessageLog(CLASS_NAME, Protocol::RAW_MAC, msg, QString("Длинна принятого пакета: %1").arg(header->len)));
                emit messageReceived(msg);
            // QCoreApplication::processEvents(); 	///< @brief Лучше убрать на Windows
													///< может быть даже вреден, если EthHandler не находится в основном потоке
            }
            else if(res == -1)
            {
                //можно добавить кол-во неудачно прочтенных пакетов или ещё че, хз
                qInfo() << "Error reading packet:" << pcap_geterr(pcapRx);
                errorOccurred = true;
                break;
            }
            else if (res == 0)
            {
               if(m_lastPacketTime.isValid() &&
                  m_lastPacketTime.msecsTo(QDateTime::currentDateTime()) > PACKET_TIMEOUT_MS)
                {
                    qInfo() << "No packets received for" << PACKET_TIMEOUT_MS/1000 << "seconds";
                   timeoutOccurred = true;
                }
                break;
            }
        }
        if (errorOccurred || timeoutOccurred) {
            // Запрашиваем остановку потока
            m_polling = false;

            // Запускаем переподключение из основного потока
            QMetaObject::invokeMethod(this, [this, errorOccurred]() {
                // Этот код выполнится в основном потоке Qt
                // Безопасно закрываем соединение
                disconnectAdapter();
                // Запускаем таймер переподключения
                startReconnectTimer();
            }, Qt::QueuedConnection);
        }
    }
    else
    {
            m_polling = false;
            // Запускаем переподключение из основного потока
            QMetaObject::invokeMethod(this, [this, errorOccurred]() {
                // Этот код выполнится в основном потоке Qt
                // Безопасно закрываем соединение
                disconnectAdapter();
                // Запускаем таймер переподключения
                startReconnectTimer();
            }, Qt::QueuedConnection);
        }
    }

#else
    if (ethSocketID == -1)
    {
        //add 16.05 BEG
        qInfo() << "Socket not initialized, attempting to reconnect";
        startReconnectTimer();
        //add 16.05 END
        return;
    }



    const size_t buf_size = appSettings().value("iface_info/eth_socket_buf_size").toULongLong();
    char buf[buf_size];



    // Чтение данных в статический буфер
    ssize_t length = recvfrom(ethSocketID, buf, buf_size, MSG_DONTWAIT, NULL, NULL);
    //add 16.05 BEG
    if (length > 0) {
        m_lastPacketTime = QDateTime::currentDateTime();
        QByteArray packet(buf, length);
        emit messageReceived(packet);
    }
    else if(length == -1)
    {
        int error_code = errno;
        if (error_code != EAGAIN && error_code != EWOULDBLOCK) {
            qWarning() << "recvfrom error:" << strerror(error_code);
            disconnectAdapter();
            startReconnectTimer();
            return;
        }
    }

    // Проверка таймаута (если долго не было пакетов)
    if (m_lastPacketTime.isValid() &&
        m_lastPacketTime.msecsTo(QDateTime::currentDateTime()) > PACKET_TIMEOUT_MS) {
        qInfo() << "No packets received for" << PACKET_TIMEOUT_MS/1000 << "seconds";
        disconnectAdapter();
        startReconnectTimer();
    }
    //add 16.05 END
    /*
    while(length > 0) {
          // Создаем QByteArray из полученных данных
        QByteArray packet(buf, length);
        emit messageReceived(packet);
        QCoreApplication::processEvents();

        // Читаем следующий пакет
        length = recvfrom(ethSocketID, buf, buf_size, MSG_DONTWAIT, NULL, NULL);
    }
    */
}
#endif

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
