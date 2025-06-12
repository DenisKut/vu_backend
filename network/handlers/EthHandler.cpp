#include "pch.h"
#include "EthHandler.h"
#include "utils/time_provider/TimeProvider.h"
#include "network/messages/EthMessage.h"
#include "utils/AppConfig.h"
#include "utils/env_loader/Settings.h"
#include <QDebug>

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
        while (m_polling && pcapRx) {
            struct pcap_pkthdr *header = nullptr;
            const unsigned char *data = nullptr;
            int res = pcap_next_ex(pcapRx, &header, &data);
            if (res == 1) {
                QByteArray msg(reinterpret_cast<const char*>(data), header->len);
                QMetaObject::invokeMethod(this, [msg, this]() {
                    processIncomingFrame(msg);
                }, Qt::QueuedConnection);
            } else if (res == -1) {
                qWarning() << "Npcap error (pcap_next_ex):" << pcap_geterr(pcapRx);
                break;
            }
            // Минимальный polling-интервал (1-2 мс)
            QThread::msleep(2);
        }
    });
    connect(m_pollThread, &QThread::finished, this, [this]() {
        if (pcapRx) { pcap_close(pcapRx); pcapRx = nullptr; }
    });
    m_pollThread->start();

    qDebug() << "Npcap polling thread started";
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

    qDebug() << "Npcap polling thread stopped";
}

#else

bool EthHandler::attachBpfFilter(int sockfd, uint16_t etherType) {
    // Фильтр BPF для указанного EtherType
    struct sock_filter bpf_code[] = {
                                     // Загружаем EtherType (2 байта по смещению 12)
                                     { 0x28, 0, 0, 0x0000000c },
                                     // Сравниваем с заданным EtherType (network byte order)
                                     { 0x15, 0, 1, static_cast<uint32_t>(htons(etherType)) },
                                     // Возвращаем весь пакет (если совпало)
                                     { 0x06, 0, 0, 0x0000ffff },
                                     // Иначе отбрасываем
                                     { 0x06, 0, 0, 0x00000000 },
                                     };

    struct sock_fprog bpf_prog = {
        .len = sizeof(bpf_code) / sizeof(bpf_code[0]),
        .filter = bpf_code,
    };

    if (setsockopt(sockfd, SOL_SOCKET, SO_ATTACH_FILTER, &bpf_prog, sizeof(bpf_prog)) == -1) {
        perror("setsockopt(SO_ATTACH_FILTER)");
        return false;
    }

    return true;
}

#endif

void EthHandler::startReconnectTimer() {
    m_reconnectTimer->start(RECONNECT_INTERVAL_MS);
}

void EthHandler::stopReconnectTimer() {
    m_reconnectTimer->stop();
    m_reconnectAttempts = 0;
}

void EthHandler::attemptReconnect() {
    m_reconnectAttempts++;
    qInfo() << "Attempting to reconnect, attempt" << m_reconnectAttempts;

    disconnectAdapter();
    QThread::msleep(500); // Даем время на корректное закрытие
    connectAdapter();
}

EthHandler::EthHandler(QObject* parent)
    : QObject(parent), m_adapterOpened(false)
{
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(false);
    connect(m_reconnectTimer, &QTimer::timeout, this, &EthHandler::attemptReconnect);
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
        strcpy(ifName, appSettings().value("iface_info/eth_interface_name")
                                            .toString().toUtf8().constData());
    }
    qDebug() << "interfaceName =" << interfaceName;
#endif
}

EthHandler::~EthHandler()
{
    disconnectAdapter();
#ifdef _WIN32
    if (pcapAllDevs) {
        pcap_freealldevs(pcapAllDevs);
    }
#endif
}

void EthHandler::freeRes()
{
#ifdef _WIN32
    if (pcapAllDevs) {
        pcap_freealldevs(pcapAllDevs);
    }
#endif
    disconnectAdapter();
}

void EthHandler::initialize()
{
    connectAdapter();
}

void EthHandler::start()
{
    connectAdapter();
}

void EthHandler::connectAdapter()
{
    if (m_adapterOpened) return;
    stopReconnectTimer();

    //тест логов
    //static int recCnt = 0;

	#ifdef _WIN32
    char errbuf[PCAP_ERRBUF_SIZE];

    if (pcap_findalldevs_ex(const_cast<char*>(PCAP_SRC_IF_STRING),
                                    nullptr, &pcapAllDevs, errbuf) == -1)
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
        qInfo() << "No suitable adapter found for EtherType 0x"
                << QString::number(m_customEtherType, 16);
#endif
        //тест логов
        //LOG_INTERNAL(CLASS_NAME, "LogLevel::Error", "ADAPTER ERROR; EthHandler::connectAdapter()");
        //LOG_PROTOCOL(CLASS_NAME, LogLevel::MessageReceived, Protocol::ETH_BINARY,QByteArray("#777#777#777") , "АБОБАБАБАБАБ");
        //LOG_INIT(CLASS_NAME, LogLevel::Error ,StatusInit::NOT_LOADED);
        /* тест смены имени файла
        recCnt++;
        if(recCnt && (recCnt % 5 == 0))
        {
            LogManager::setFileName("session_" + QString::number(recCnt / 5) + "_logs.bin");
        }
        */
        freeRes();
        startReconnectTimer();
        return;
    }

#ifdef QT_DEBUG
    qInfo() << "Choosen adapter: " << d->name;
#endif

    // Подключить передатчик к выбранному адаптеру:
    //------------------------------------------------------------------
    if ( (pcapTx = pcap_open(d->name,        2048,        PCAP_OPENFLAG_PROMISCUOUS |
        PCAP_OPENFLAG_MAX_RESPONSIVENESS |
        PCAP_OPENFLAG_NOCAPTURE_LOCAL,       10,       nullptr,       errbuf    ) ) == nullptr)
    {
#ifdef QT_DEBUG
        qDebug() << "Failed to open pcapTx";
#endif

        LOG_INTERNAL(CLASS_NAME, "MAC SOCKET", "WINPCAP ERROR; EthHandler::connectAdapter() - Не получилось открыть pcapTx");

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
        LOG(InternalMessageLog,CLASS_NAME, "MAC SOCKET", "WINPCAP; pcapTx открыт");
    }

    pcap_setbuff(pcapTx, 160*1024*1024);
    pcap_setnonblock(pcapTx, 0, errbuf);
    pcap_setmintocopy(pcapTx, 1); // Minimize latency for TX
    //------------------------------------------------------------------

    // Подключить приёмник к выбранному адаптеру:
    //------------------------------------------------------------------
    if ( (pcapRx = pcap_open(d->name,        2048,        PCAP_OPENFLAG_PROMISCUOUS |
        PCAP_OPENFLAG_MAX_RESPONSIVENESS |
        PCAP_OPENFLAG_NOCAPTURE_LOCAL,       10,       nullptr,       errbuf    ) ) == nullptr)
    {
#ifdef QT_DEBUG
        qDebug() << "Failed to open pcapRx";
#endif
        LOG_INTERNAL(CLASS_NAME, "MAC SOCKET", "WINPCAP ERROR; EthHandler::connectAdapter() - Не получилось открыть pcapTx");
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
        LOG(InternalMessageLog,CLASS_NAME, "MAC SOCKET","WINPCAP; pcapTx открыт");
    }

    pcap_setbuff(pcapRx, 160*1024*1024);
    pcap_setnonblock(pcapRx, 1, errbuf); // 1 - неблокирующий режим
    pcap_setmintocopy(pcapRx, 1); // Minimize latency for RX
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
    LOG_INTERNAL(CLASS_NAME, "MAC SOCKET", "WINPCAP; " + ds);

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

    if (!attachBpfFilter(ethSocketID, ETH_TYPE)) {
        close(ethSocketID);
        ethSocketID = -1;
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
        LOG(InternalMessageLog,CLASS_NAME, "MAC SOCKET",
        "ОШИБКА : EthHandler::connectAdapter() - SO_BINDTODEVICE ");
        close(ethSocketID);
        ethSocketID = -1;
        startReconnectTimer();
        return;
    }

    qDebug() << "Starting capture on interface" << ifName << "...\n";
    LOG(InternalMessageLog,CLASS_NAME, "MAC SOCKET", QString("Подключен Ethernet адаптер: %1").arg(ifName));

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
    LOG_INTERNAL(CLASS_NAME, "MAC SOCKET", "WIN32; Ошибка при отправке Ethernet-пакета");
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
    bool errorOccurred = false;
    bool timeoutOccurred = false;
    Q_UNUSED(errorOccurred);
    Q_UNUSED(timeoutOccurred);

#ifdef _WIN32
    struct pcap_pkthdr *header;
    const unsigned char *data;
    int res = 0;


    if (pcapRx)
    {
        while ((res = pcap_next_ex(pcapRx, &header, &data)))
        {
            if(res == 1)
            {
                m_lastPacketTime = QDateTime::currentDateTime();

                QByteArray msg(reinterpret_cast<const char*>(data), header->len);
                LOG(ProtocolMessageLog,CLASS_NAME, Protocol::ETH_BINARY, msg, QString("Длинна принятого пакета: %1").arg(header->len));
                processIncomingFrame(msg); // Важно: обработка каждого кадра
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
            QMetaObject::invokeMethod(this, [this]() {
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
            QMetaObject::invokeMethod(this, [this]() {
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
        qInfo() << "Socket not initialized, attempting to reconnect";
        startReconnectTimer();
        return;
    }



    const size_t buf_size = appSettings().value("iface_info/eth_socket_buf_size").toULongLong();
    char buf[buf_size];



    // Чтение данных в статический буфер
    ssize_t length = recvfrom(ethSocketID, buf, buf_size, MSG_DONTWAIT, NULL, NULL);
    if (length > 0) {
        m_lastPacketTime = QDateTime::currentDateTime();
        QByteArray packet(buf, length);
        processIncomingFrame(packet); // Важно: обработка каждого кадра
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

void EthHandler::processIncomingFrame(const QByteArray& msg) {
    // Вся логика из NetworkManager::onEthReceived
    // qInfo() << "processIncomingFrame";
    auto parsed = EthMessageFactory::parse(msg);
    if (parsed)
    {
#ifdef QT_DEBUG
        qDebug() << "Captured ETH Message" << Qt::endl;
#endif
#ifdef TRACE
        qDebug() << Qt::endl << "ETH Message:" << msg.toHex(' ').toUpper();
        qDebug() << "parsed->mesNumber =" << TO_HEX_STR(&parsed->mesNumber, 2);
#endif
        // BS_SYNC (2.5)
        if(parsed->mesNumber == to_underlying(ETH_MessageNumbers::BS_SYNC)) {
            auto msgBS_SYNC = reinterpret_cast<EthMessage_BS_SYNC*>(parsed.get());
            if (msgBS_SYNC)
            {
                TimeProvider::instance().setBsPeriodNumber(msgBS_SYNC->periodNumber);
                TimeProvider::instance().setBsPeriodTime(msgBS_SYNC->periodTime);
                TimeProvider::instance().setBsPeriodWidth(msgBS_SYNC->periodWidth);
#ifdef QT_DEBUG
                qDebug() << "2.5 RECEIVED";
#endif
#ifdef TRACE
                qDebug() << "--------------------------------------------";
                qDebug() << "timeReceive =" << TO_HEX_STR(msgBS_SYNC->timeReceive, 5);
                qDebug() << "timeBeginTrans =" << TO_HEX_STR(msgBS_SYNC->timeBeginTrans, 5);
                qDebug() << "periodNumber =" << TO_HEX_STR(&msgBS_SYNC->periodNumber, 4);
                qDebug() << "periodTime =" << TO_HEX_STR(msgBS_SYNC->periodTime, 5);
                qDebug() << "periodWidth =" << TO_HEX_STR(&msgBS_SYNC->periodWidth, 4);
                qDebug() << "--------------------------------------------";
#endif
                LOG_PROTOCOL(CLASS_NAME, LogLevel::MessageReceived, Protocol::ETH_BINARY, msg, LogManager::jobj_toString(msgBS_SYNC->to_json()));
            }
        }
        // PPU_POSITION_1 (11.1)
        else if(parsed->mesNumber == to_underlying(ETH_MessageNumbers::PPU_POSITION_1)) {
            auto msgPPU_POSITION = reinterpret_cast<EthMessage_PPU_POSITION_1*>(parsed.get());
            if (msgPPU_POSITION)
            {
                int64_t timeTemp = 0;
                for (int i = 4; i >= 0; --i) {
                    timeTemp = (timeTemp << 8) | msgPPU_POSITION->timeBeginTrans[i];
                }
                timeTemp &= 0x000000FFFFFFFFFF;
                TimeProvider::instance().setBsTimePPU(timeTemp);
                AppConfig::instance().setModulePPU_lastCommand(AppConfig::ModulePPU_num_RLMN_1, msgPPU_POSITION->lastCommand);
                AppConfig::instance().setModulePPU_state(AppConfig::ModulePPU_num_RLMN_1, msgPPU_POSITION->state);
                AppConfig::instance().setModulePPU_ispr(AppConfig::ModulePPU_num_RLMN_1, msgPPU_POSITION->ispr);
                AppConfig::instance().setModulePPU_az(AppConfig::ModulePPU_num_RLMN_1, msgPPU_POSITION->az);
                AppConfig::instance().setModulePPU_azDelay(AppConfig::ModulePPU_num_RLMN_1, msgPPU_POSITION->azDelay);
                AppConfig::instance().setModulePPU_an(AppConfig::ModulePPU_num_RLMN_1, msgPPU_POSITION->an);
                AppConfig::instance().setModulePPU_anDelay(AppConfig::ModulePPU_num_RLMN_1, msgPPU_POSITION->anDelay);
                AppConfig::instance().setModulePPU_borders(AppConfig::ModulePPU_num_RLMN_1, msgPPU_POSITION->borders);
                AppConfig::instance().setModulePPU_waitCommands(AppConfig::ModulePPU_num_RLMN_1, msgPPU_POSITION->waitCommands);
                AppConfig::instance().setModulePPU_waitTarget(AppConfig::ModulePPU_num_RLMN_1, msgPPU_POSITION->waitTarget);
#ifdef QT_DEBUG
                qDebug() << "11.1 RECEIVED";
#endif
#ifdef TRACE
                qDebug() << "--------------------------------------------";
                qDebug() << "timeReceive =" << TO_HEX_STR(msgPPU_POSITION->timeReceive, 5);
                qDebug() << "timeBeginTrans =" << TO_HEX_STR(msgPPU_POSITION->timeBeginTrans, 5);
                qDebug() << "lastCommand =" << TO_HEX_STR(&msgPPU_POSITION->lastCommand, 1);
                qDebug() << "state =" << TO_HEX_STR(&msgPPU_POSITION->state, 1);
                qDebug() << "ispr =" << TO_HEX_STR(&msgPPU_POSITION->ispr, 1);
                qDebug() << "az =" << TO_HEX_STR(msgPPU_POSITION->az, 3);
                qDebug() << "azDelay =" << TO_HEX_STR(&msgPPU_POSITION->azDelay, 1);
                qDebug() << "an =" << TO_HEX_STR(msgPPU_POSITION->an, 3);
                qDebug() << "anDelay =" << TO_HEX_STR(&msgPPU_POSITION->anDelay, 1);
                qDebug() << "borders =" << TO_HEX_STR(&msgPPU_POSITION->borders, 1);
                qDebug() << "waitCommands =" << TO_HEX_STR(&msgPPU_POSITION->waitCommands, 1);
                qDebug() << "waitTarget =" << TO_HEX_STR(&msgPPU_POSITION->waitTarget, 1);
                qDebug() << "--------------------------------------------";
#endif
                LOG_PROTOCOL(CLASS_NAME, LogLevel::MessageReceived, Protocol::ETH_BINARY, msg, LogManager::jobj_toString(msgPPU_POSITION->to_json()));
#ifdef QT_DEBUG
                qDebug() << "PPU 1";
#endif 
#ifdef TRACE
                qDebug() << "--------------------------------------------";
                qDebug() << "lastCommand =" << AppConfig::instance().getModulePPU_lastCommand(AppConfig::ModulePPU_num_RLMN_1);
                qDebug() << "state =" << AppConfig::instance().getModulePPU_state(AppConfig::ModulePPU_num_RLMN_1);
                qDebug() << "ispr =" << AppConfig::instance().getModulePPU_ispr(AppConfig::ModulePPU_num_RLMN_1);
                qDebug() << "az =" << AppConfig::instance().getModulePPU_az(AppConfig::ModulePPU_num_RLMN_1);
                qDebug() << "azDelay =" << AppConfig::instance().getModulePPU_azDelay(AppConfig::ModulePPU_num_RLMN_1);
                qDebug() << "an =" << AppConfig::instance().getModulePPU_an(AppConfig::ModulePPU_num_RLMN_1);
                qDebug() << "anDelay =" << AppConfig::instance().getModulePPU_anDelay(AppConfig::ModulePPU_num_RLMN_1);
                qDebug() << "borders =" << AppConfig::instance().getModulePPU_borders(AppConfig::ModulePPU_num_RLMN_1);
                qDebug() << "waitCommands =" << AppConfig::instance().getModulePPU_waitCommands(AppConfig::ModulePPU_num_RLMN_1);
                qDebug() << "waitTarget =" << AppConfig::instance().getModulePPU_waitTarget(AppConfig::ModulePPU_num_RLMN_1);
                qDebug() << "--------------------------------------------";
                qDebug() << "getBsPeriodNumber =" << TimeProvider::instance().getBsPeriodNumber();
                qDebug() << "getBsPeriodTime =" << TimeProvider::instance().getBsPeriodTime();
                qDebug() << "getBsPeriodWidth =" << TimeProvider::instance().getBsPeriodWidth();
#endif
                uint8_t state = AppConfig::instance().getModulePPU_state(AppConfig::ModulePPU_num_RLMN_1);
                if (state & AppConfig::ModulePPU_state_TurnedOn)
                {
#ifdef QT_DEBUG
                    qDebug() << "PPU 1 is turned On" << Qt::endl;
#endif
                    static double azC0 = 0, anC0 = 0, azC1 = 0, anC1 = 0, azC2 = 0, anC2 = 0;
                    static uint16_t azC1u = 0, azC2u = 0, anC1u = 0, anC2u = 0;
                    static uint32_t azC0u = 0, anC0u = 0;
                    static int mode = 3;
                    auto msg = EthMessageFactory::create<EthMessage_PPU_MOVE_1>();
                    if (mode == 0)
                    {
                        static bool isChangedImitPeriod = false;
                        if (!isChangedImitPeriod) {
                            const u_char buf[60] = {0x00, 0x0A, 0x35, 0x00, 0x23, 0x03, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x20, 0x20, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x04, 0x06, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x28, 0x00, 0x19, 0x00, 0x05, 0x00, 0x64, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                            QByteArray data = QByteArray(reinterpret_cast<const char*>(buf), sizeof(buf));
#ifdef QT_DEBUG
                            qDebug() << "Sending message to change period of messages on imitator:";
#endif
#ifdef TRACE
                            qDebug() << "ETH Message from VU to BS:" << Qt::endl << data.toHex(' ').toUpper();
#endif
                            QMetaObject::invokeMethod(this, "sendMessage", Qt::QueuedConnection, Q_ARG(QByteArray, data));
                            isChangedImitPeriod = true;
                            mode = 1;
                        }
                    }
                    else if (mode == 1) { mode = 2; }
                    else if (mode == 2) { mode = 3; }
                    else if (mode == 3) {
                        mode = 4;
//                        double dt = 0.0000001*static_cast<double>(TimeProvider::instance().getBsPeriodWidth());
                        // double dt = 0.1;
                        // double az = AppConfig::instance().getModulePPU_az(AppConfig::ModulePPU_num_RLMN_1);
                        // double an = AppConfig::instance().getModulePPU_an(AppConfig::ModulePPU_num_RLMN_1);
                        // static double azLast = 0, azAccLast = 0, anLast = 0, anAccLast = 0;
                        // double aMax = 30.0, KP = 3.0, KD = 6.0;
                        // dt *= 3.0;
                        double azTarget = AppConfig::instance().getModulePPU_azTarget(AppConfig::ModulePPU_num_RLMN_1);
                        double anTarget = AppConfig::instance().getModulePPU_anTarget(AppConfig::ModulePPU_num_RLMN_1);
                        // double azTargetVel = AppConfig::instance().getModulePPU_azTargetVel(AppConfig::ModulePPU_num_RLMN_1);
                        // double anTargetVel = AppConfig::instance().getModulePPU_anTargetVel(AppConfig::ModulePPU_num_RLMN_1);
//                        calculateCoefficientsPPU(azTarget, azTargetVel, az, &azLast, &azAccLast, aMax, KP, KD, dt, &azC0, &azC1, &azC2);
//                        calculateCoefficientsPPU(anTarget, anTargetVel, an, &anLast, &anAccLast, aMax, KP, KD, dt, &anC0, &anC1, &anC2);
//                        azC2 *= 2.0; anC2 *= 2.0;

                        // Vremenno peredaiom fiksirovannie koordinaty
                        //------------------------------------------------------
                        azC0 = azTarget;
                        anC0 = anTarget;
                        if (anC0 < 0.01) anC0 = 0.01;
                        azC1 = AppConfig::instance().getModulePPU_azC1(AppConfig::ModulePPU_num_RLMN_1);
                        anC1 = AppConfig::instance().getModulePPU_anC1(AppConfig::ModulePPU_num_RLMN_1);
                        azC2 = AppConfig::instance().getModulePPU_azC2(AppConfig::ModulePPU_num_RLMN_1);
                        anC2 = AppConfig::instance().getModulePPU_anC2(AppConfig::ModulePPU_num_RLMN_1);
                        //------------------------------------------------------

                        static constexpr double INV_SCALE = 65536.0 / 360.0;
                        azC0u = static_cast<uint32_t>(azC0 * INV_SCALE);
                        azC1u = static_cast<uint16_t>(azC1 * INV_SCALE);
                        azC2u = static_cast<uint16_t>(azC2 * INV_SCALE);
                        anC0u = static_cast<uint32_t>(anC0 * INV_SCALE);
                        anC1u = static_cast<uint16_t>(anC1 * INV_SCALE);
                        anC2u = static_cast<uint16_t>(anC2 * INV_SCALE);
#ifdef QT_DEBUG
                        qDebug() << "azCur =" << azC0;
                        qDebug() << "azVel =" << azC1 << "=" << azC1u;
                        qDebug() << "azAcc =" << 2.0 * azC2 << "=" << azC2u;
                        qDebug() << "anCur =" << anC0;
                        qDebug() << "anVel =" << anC1 << "=" << anC1u;
                        qDebug() << "anAcc =" << 2.0 * anC2 << "=" << anC2u;
#endif
                        msg.azC0[0] = static_cast<uint8_t>(azC0u);
                        msg.azC0[1] = static_cast<uint8_t>(azC0u >> 8);
                        msg.azC0[2] = static_cast<uint8_t>(azC0u >> 16);
                        msg.azC1 = azC1u;
                        msg.azC2 = azC2u;

                        msg.anC0[0] = static_cast<uint8_t>(anC0u);
                        msg.anC0[1] = static_cast<uint8_t>(anC0u >> 8);
                        msg.anC0[2] = static_cast<uint8_t>(anC0u >> 16);
                        msg.anC1 = anC1u;
                        msg.anC2 = anC2u;

                        msg.azSector = 0;
                        msg.anSector = 0;

                        QByteArray data = msg.serializeFull();
#ifdef QT_DEBUG
                        qDebug() << "Sending message to move the PPU 1";
#endif 
#ifdef TRACE
                        qDebug() << "ETH Message from VU to BS:" << Qt::endl << data.toHex(' ').toUpper();
#endif
                        sendMessage(data);
                        LOG_PROTOCOL(CLASS_NAME, LogLevel::MessageSent, Protocol::ETH_BINARY, data.toHex(' ').toUpper(), LogManager::jobj_toString(msg.to_json()));
                    }
                    else if (mode == 4) { mode = 5; }
                    else if (mode == 5) { mode = 3; }
//                    else if (mode == 6) { mode = 7; }
//                    else if (mode == 7) { mode = 8; }
//                    else if (mode == 8) { mode = 9; }
//                    else if (mode == 9) { mode = 10; }
//                    else if (mode == 10) { mode = 11; }
//                    else if (mode == 11) { mode = 12; }
//                    else if (mode == 12) { mode = 3; }

                    // Отправка отладочных данных на форму для отображения:
                    /*
                    RmoMessage_TargetPPU msgRmo;
                    msgRmo.azPPU = AppConfig::instance().getModulePPU_az(AppConfig::ModulePPU_num_RLMN_1);
                    msgRmo.anPPU = AppConfig::instance().getModulePPU_an(AppConfig::ModulePPU_num_RLMN_1);
                    msgRmo.azC0 = azC0; msgRmo.azC1 = azC1; msgRmo.azC2 = azC2;
                    msgRmo.anC0 = anC0; msgRmo.anC1 = anC1; msgRmo.anC2 = anC2;
                    msgRmo.azTarget = AppConfig::instance().getModulePPU_azTarget(AppConfig::ModulePPU_num_RLMN_1);
                    msgRmo.anTarget = AppConfig::instance().getModulePPU_anTarget(AppConfig::ModulePPU_num_RLMN_1);
                    msgRmo.azTargetVel = AppConfig::instance().getModulePPU_azTargetVel(AppConfig::ModulePPU_num_RLMN_1);
                    msgRmo.anTargetVel = AppConfig::instance().getModulePPU_anTargetVel(AppConfig::ModulePPU_num_RLMN_1);
                    msgRmo.isRemote = static_cast<uint8_t>(AppConfig::instance().getModulePPU_isRemote(AppConfig::ModulePPU_num_RLMN_1));
                    msgRmo.counter = rmoCounter++;
                    */
                    //auto msgRmo = QSharedPointer::create<RmoMessage_TargetPPU>();

                    auto msgFill = RmoMessageFactory::instance()
                           .createWithArgs<RmoMessage_TargetPPU>(
                               AppConfig::ModulePPU_num::ModulePPU_num_RLMN_1
                            );
                    msgFill->set_azC(azC0, azC1, azC2);
                    msgFill->set_anC(anC0, anC1, anC2);
                    msgFill->counter = AppConfig::instance().increaseRmoCounter();

                    if (m_tcpSendCallback) m_tcpSendCallback(msgFill);
                }
                else
                {
#ifdef QT_DEBUG
                    qDebug() << "PPU 1 is turned Off" << Qt::endl;
#endif
                    auto msgEnable = EthMessageFactory::create<EthMessage_PPU_ENABLE_1>();
                    msgEnable.state = AppConfig::ModulePPU_state_TurnedOn; // Включить ППУ
                    QByteArray data = msgEnable.serializeFull();
#ifdef QT_DEBUG
                    qDebug() << "Sending message to turn on the PPU";
#endif
#ifdef TRACE
                    qDebug() << "ETH Message from VU to BS:" << Qt::endl << data.toHex(' ').toUpper();
#endif
                    sendMessage(data);
                    LOG(ProtocolMessageLog,CLASS_NAME, LogLevel::MessageSent, Protocol::ETH_BINARY, data.toHex(' ').toUpper(), LogManager::jobj_toString(msgEnable.to_json()));
                }
            }
        }
        // PPU_POSITION_2 (11.11)
        else if(parsed->mesNumber == to_underlying(ETH_MessageNumbers::PPU_POSITION_2)) {
            auto msgPPU_POSITION = reinterpret_cast<EthMessage_PPU_POSITION_2*>(parsed.get());
            if (msgPPU_POSITION)
            {
                int64_t timeTemp = 0;
                for (int i = 4; i >= 0; --i) {
                    timeTemp = (timeTemp << 8) | msgPPU_POSITION->timeBeginTrans[i];
                }
                timeTemp &= 0x000000FFFFFFFFFF;
                TimeProvider::instance().setBsTimePPU(timeTemp);
                AppConfig::instance().setModulePPU_lastCommand(AppConfig::ModulePPU_num_RLMN_2, msgPPU_POSITION->lastCommand);
                AppConfig::instance().setModulePPU_state(AppConfig::ModulePPU_num_RLMN_2, msgPPU_POSITION->state);
                AppConfig::instance().setModulePPU_ispr(AppConfig::ModulePPU_num_RLMN_2, msgPPU_POSITION->ispr);
                AppConfig::instance().setModulePPU_az(AppConfig::ModulePPU_num_RLMN_2, msgPPU_POSITION->az);
                AppConfig::instance().setModulePPU_azDelay(AppConfig::ModulePPU_num_RLMN_2, msgPPU_POSITION->azDelay);
                AppConfig::instance().setModulePPU_an(AppConfig::ModulePPU_num_RLMN_2, msgPPU_POSITION->an);
                AppConfig::instance().setModulePPU_anDelay(AppConfig::ModulePPU_num_RLMN_2, msgPPU_POSITION->anDelay);
                AppConfig::instance().setModulePPU_borders(AppConfig::ModulePPU_num_RLMN_2, msgPPU_POSITION->borders);
                AppConfig::instance().setModulePPU_waitCommands(AppConfig::ModulePPU_num_RLMN_2, msgPPU_POSITION->waitCommands);
                AppConfig::instance().setModulePPU_waitTarget(AppConfig::ModulePPU_num_RLMN_2, msgPPU_POSITION->waitTarget);
#ifdef QT_DEBUG
                qDebug() << "11.11 RECEIVED";
#endif
#ifdef TRACE
                qDebug() << "--------------------------------------------";
                qDebug() << "timeReceive =" << TO_HEX_STR(msgPPU_POSITION->timeReceive, 5);
                qDebug() << "timeBeginTrans =" << TO_HEX_STR(msgPPU_POSITION->timeBeginTrans, 5);
                qDebug() << "lastCommand =" << TO_HEX_STR(&msgPPU_POSITION->lastCommand, 1);
                qDebug() << "state =" << TO_HEX_STR(&msgPPU_POSITION->state, 1);
                qDebug() << "ispr =" << TO_HEX_STR(&msgPPU_POSITION->ispr, 1);
                qDebug() << "az =" << TO_HEX_STR(msgPPU_POSITION->az, 3);
                qDebug() << "azDelay =" << TO_HEX_STR(&msgPPU_POSITION->azDelay, 1);
                qDebug() << "an =" << TO_HEX_STR(msgPPU_POSITION->an, 3);
                qDebug() << "anDelay =" << TO_HEX_STR(&msgPPU_POSITION->anDelay, 1);
                qDebug() << "borders =" << TO_HEX_STR(&msgPPU_POSITION->borders, 1);
                qDebug() << "waitCommands =" << TO_HEX_STR(&msgPPU_POSITION->waitCommands, 1);
                qDebug() << "waitTarget =" << TO_HEX_STR(&msgPPU_POSITION->waitTarget, 1);
                qDebug() << "--------------------------------------------";
#endif
                LOG(ProtocolMessageLog,CLASS_NAME, LogLevel::MessageReceived, Protocol::ETH_BINARY, msg, LogManager::jobj_toString(msgPPU_POSITION->to_json()));
#ifdef QT_DEBUG
                qDebug() << "PPU 2";
#endif
#ifdef TRACE
                qDebug() << "--------------------------------------------";
                qDebug() << "lastCommand =" << AppConfig::instance().getModulePPU_lastCommand(AppConfig::ModulePPU_num_RLMN_2);
                qDebug() << "state =" << AppConfig::instance().getModulePPU_state(AppConfig::ModulePPU_num_RLMN_2);
                qDebug() << "ispr =" << AppConfig::instance().getModulePPU_ispr(AppConfig::ModulePPU_num_RLMN_2);
                qDebug() << "az =" << AppConfig::instance().getModulePPU_az(AppConfig::ModulePPU_num_RLMN_2);
                qDebug() << "azDelay =" << AppConfig::instance().getModulePPU_azDelay(AppConfig::ModulePPU_num_RLMN_2);
                qDebug() << "an =" << AppConfig::instance().getModulePPU_an(AppConfig::ModulePPU_num_RLMN_2);
                qDebug() << "anDelay =" << AppConfig::instance().getModulePPU_anDelay(AppConfig::ModulePPU_num_RLMN_2);
                qDebug() << "borders =" << AppConfig::instance().getModulePPU_borders(AppConfig::ModulePPU_num_RLMN_2);
                qDebug() << "waitCommands =" << AppConfig::instance().getModulePPU_waitCommands(AppConfig::ModulePPU_num_RLMN_2);
                qDebug() << "waitTarget =" << AppConfig::instance().getModulePPU_waitTarget(AppConfig::ModulePPU_num_RLMN_2);
                qDebug() << "--------------------------------------------";
                qDebug() << "getBsPeriodNumber =" << TimeProvider::instance().getBsPeriodNumber();
                qDebug() << "getBsPeriodTime =" << TimeProvider::instance().getBsPeriodTime();
                qDebug() << "getBsPeriodWidth =" << TimeProvider::instance().getBsPeriodWidth();
#endif
                uint8_t state = AppConfig::instance().getModulePPU_state(AppConfig::ModulePPU_num_RLMN_2);
                if (state & AppConfig::ModulePPU_state_TurnedOn)
                {
#ifdef QT_DEBUG
                    qDebug() << "PPU 2 is turned On" << Qt::endl;
#endif
                    static double azC0 = 0, anC0 = 0, azC1 = 0, anC1 = 0, azC2 = 0, anC2 = 0;
                    static uint16_t azC1u = 0, azC2u = 0, anC1u = 0, anC2u = 0;
                    static uint32_t azC0u = 0, anC0u = 0;
                    static int mode = 3;
                    auto msg = EthMessageFactory::create<EthMessage_PPU_MOVE_2>();
                    if (mode == 0)
                    {
                        static bool isChangedImitPeriod = false;
                        if (!isChangedImitPeriod) {
                            const u_char buf[60] = {0x00, 0x0A, 0x35, 0x00, 0x23, 0x03, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x20, 0x20, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x04, 0x06, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x28, 0x00, 0x19, 0x00, 0x05, 0x00, 0x64, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                            QByteArray data = QByteArray(reinterpret_cast<const char*>(buf), sizeof(buf));
#ifdef QT_DEBUG
                            qDebug() << "Sending message to change period of messages on imitator:";
#endif
#ifdef TRACE
                            qDebug() << "ETH Message from VU to BS:" << Qt::endl << data.toHex(' ').toUpper();
#endif
                            QMetaObject::invokeMethod(this, "sendMessage", Qt::QueuedConnection, Q_ARG(QByteArray, data));
                            isChangedImitPeriod = true;
                            mode = 1;
                        }
                    }
                    else if (mode == 1) { mode = 2; }
                    else if (mode == 2) { mode = 3; }
                    else if (mode == 3) {
                        mode = 4;
                        double azTarget = AppConfig::instance().getModulePPU_azTarget(AppConfig::ModulePPU_num_RLMN_2);
                        double anTarget = AppConfig::instance().getModulePPU_anTarget(AppConfig::ModulePPU_num_RLMN_2);

                        //------------------------------------------------------
                        azC0 = azTarget;
                        anC0 = anTarget;
                        if (anC0 < 0.01) anC0 = 0.01;
                        azC1 = AppConfig::instance().getModulePPU_azC1(AppConfig::ModulePPU_num_RLMN_2);
                        anC1 = AppConfig::instance().getModulePPU_anC1(AppConfig::ModulePPU_num_RLMN_2);
                        azC2 = AppConfig::instance().getModulePPU_azC2(AppConfig::ModulePPU_num_RLMN_2);
                        anC2 = AppConfig::instance().getModulePPU_anC2(AppConfig::ModulePPU_num_RLMN_2);
                        //------------------------------------------------------

                        static constexpr double INV_SCALE = 65536.0 / 360.0;
                        azC0u = static_cast<uint32_t>(azC0 * INV_SCALE);
                        azC1u = static_cast<uint16_t>(azC1 * INV_SCALE);
                        azC2u = static_cast<uint16_t>(azC2 * INV_SCALE);
                        anC0u = static_cast<uint32_t>(anC0 * INV_SCALE);
                        anC1u = static_cast<uint16_t>(anC1 * INV_SCALE);
                        anC2u = static_cast<uint16_t>(anC2 * INV_SCALE);
#ifdef QT_DEBUG
                        qDebug() << "azCur =" << azC0;
                        qDebug() << "azVel =" << azC1 << "=" << azC1u;
                        qDebug() << "azAcc =" << 2.0 * azC2 << "=" << azC2u;
                        qDebug() << "anCur =" << anC0;
                        qDebug() << "anVel =" << anC1 << "=" << anC1u;
                        qDebug() << "anAcc =" << 2.0 * anC2 << "=" << anC2u;
#endif
                        msg.azC0[0] = static_cast<uint8_t>(azC0u);
                        msg.azC0[1] = static_cast<uint8_t>(azC0u >> 8);
                        msg.azC0[2] = static_cast<uint8_t>(azC0u >> 16);
                        msg.azC1 = azC1u;
                        msg.azC2 = azC2u;

                        msg.anC0[0] = static_cast<uint8_t>(anC0u);
                        msg.anC0[1] = static_cast<uint8_t>(anC0u >> 8);
                        msg.anC0[2] = static_cast<uint8_t>(anC0u >> 16);
                        msg.anC1 = anC1u;
                        msg.anC2 = anC2u;

                        msg.azSector = 0;
                        msg.anSector = 0;

                        QByteArray data = msg.serializeFull();
#ifdef QT_DEBUG
                        qDebug() << "Sending message to move the PPU 2";
#endif
#ifdef TRACE
                        qDebug() << "ETH Message from VU to BS:" << Qt::endl << data.toHex(' ').toUpper();
#endif
                        sendMessage(data);
                        LOG(ProtocolMessageLog,CLASS_NAME, LogLevel::MessageSent, Protocol::ETH_BINARY, data.toHex(' ').toUpper(), LogManager::jobj_toString(msg.to_json()));
                    }
                    else if (mode == 4) { mode = 5; }
                    else if (mode == 5) { mode = 3; }
                    //                    else if (mode == 6) { mode = 7; }
                    //                    else if (mode == 7) { mode = 8; }
                    //                    else if (mode == 8) { mode = 9; }
                    //                    else if (mode == 9) { mode = 10; }
                    //                    else if (mode == 10) { mode = 11; }
                    //                    else if (mode == 11) { mode = 12; }
                    //                    else if (mode == 12) { mode = 3; }

                    // Отправка отладочных данных на форму для отображения:
                    auto msgFill = RmoMessageFactory::instance()
                            .createWithArgs<RmoMessage_TargetPPU>(
                                AppConfig::ModulePPU_num::ModulePPU_num_RLMN_2
                            );
                    msgFill->set_azC(azC0, azC1, azC2);
                    msgFill->set_anC(anC0, anC1, anC2);
                    msgFill->counter = AppConfig::instance().increaseRmoCounter();

                    if (m_tcpSendCallback) m_tcpSendCallback(msgFill);
                }
                else
                {
#ifdef QT_DEBUG
                    qDebug() << "PPU 2 is turned Off" << Qt::endl;
#endif
                    auto msgEnable = EthMessageFactory::create<EthMessage_PPU_ENABLE_2>();
                    msgEnable.state = AppConfig::ModulePPU_state_TurnedOn; // Включить ППУ
                    QByteArray data = msgEnable.serializeFull();
#ifdef QT_DEBUG
                    qDebug() << "Sending message to turn on the PPU 2";
#endif
#ifdef TRACE
                    qDebug() << "ETH Message from VU to BS:" << Qt::endl << data.toHex(' ').toUpper();
#endif
                    sendMessage(data);
                    LOG_PROTOCOL(CLASS_NAME, LogLevel::MessageSent, Protocol::ETH_BINARY, data.toHex(' ').toUpper(), LogManager::jobj_toString(msgEnable.to_json()));
                }
            }
        }
        // ...other message types as needed...
    }
}

// ППУ:
//----------------------------------------------------------
void EthHandler::calculateCoefficientsPPU(double angleTarget,
                              double velocityTarget,
                              double angleCurrent, double* angleLast,
                              double* accelerationLast, double accelerationMax,
                              // double angleMin, double velocityMin,
                              double KP, double KD, /*double dtLast,*/double dt,
                              double* C0, double* C1, double* C2) {


    double xDist = angleTarget - angleCurrent;
    if (xDist > 180.0) {
        xDist -= 360.0;
    } else if (xDist < -180.0) {
        xDist += 360.0;
    }
    double dx = angleCurrent - (*angleLast);
    if (dx > 180.0) {
        dx -= 360.0;
    } else if (dx < -180.0) {
        dx += 360.0;
    }
    double v1 = dx / dt;// + 0.5 * (*accelerationLast) * dt;
    double dv = velocityTarget - v1;
    double a1 = 0.0;

    //        double breaking_dist = dv * dv / (2.0 * accelerationMax) + angleMin;

    //        if (abs(xDist) > angleMin) {
    //            // Максимальное ускорение в направлении цели
    //            a1 = (xDist > 0) ? accelerationMax : -accelerationMax;

    //            // Если расстояние торможения достигнуто, то тормозим:
    //            if (abs(xDist) < breaking_dist) {
    //                a1 = (dv < 0) ? -accelerationMax : accelerationMax;
    //            }

    //            // Плавное изменение ускорения:
    //            if (abs(dv) < velocityMin) {
    //                a1 = (*accelerationLast) + 0.5 * (a1 - (*accelerationLast));
    //            }
    //        } else {
    // ПД-регулятор
    a1 = KP * xDist + KD * dv;
    //        }

    // Ограничение ускорения
    a1 = fmax(fmin(a1, accelerationMax), -accelerationMax);

    //        // Плавное изменение ускорения:
    //        if (abs(dv) < velocityMin) {
    //            a1 = (*accelerationLast) + 0.8 * (a1 - (*accelerationLast));
    //        }

    // Запоминаем предыдущие значения положения и ускорения точки 1
    *angleLast = angleCurrent;
    *accelerationLast = a1;

    // Установка коэффициентов
    *C0 = angleCurrent; // Начальное смещение (не используется)
    *C1 = v1;           // Текущая скорость (начальная для интервала)
    *C2 = 0.5 * a1;     // Ускорение, деленное на 2
}
//----------------------------------------------------------
void EthHandler::setTcpSendCallback(std::function<void(QSharedPointer<RmoBaseMessage>)> cb) {
    m_tcpSendCallback = std::move(cb);
}
