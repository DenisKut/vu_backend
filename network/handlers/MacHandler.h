#ifndef ETHHANDLER_H
#define ETHHANDLER_H

#include <QTimer>
#include <QObject>
#include <QByteArray>
#include <QDebug>
#include <QDataStream>
#include "../messages/Message.h"
#include "utils/TimeProvider.h"

// Библиотека WinPCap для работы с
// Ethernet raw data под Windows:
#ifdef _WIN32
#include <pcap.h>
#endif

// Структура MAC пакета:
struct TMacPacket
{
    long long timeStump;                    // Метка времени приёма пакета
    QByteArray data;
};

class MacHandler : public QObject {
    Q_OBJECT

public:
    explicit MacHandler(TimeProvider* timeProvider, QObject* parent = nullptr);
    ~MacHandler();

    QVector<TMacPacket*> vecPackets; // Данные MAC пакетов

    // Адаптеры FastEthernet:
    //---------------------------------------------------------------------
    pcap_if_t *pcapAllDevs;                 // Список доступных адаптеров
    pcap_t *pcapTx;                         // Передатчик
    pcap_t *pcapRx;                         // Приёмник

    QStringList strAdapterNames;            // Список имён подключённых адаптеров
    QVector<QString> getNamesAdapters();    // Возвращает список имён существующих адаптеров Fast Ethernet
    void connectAdapter(int inum);          // Подключиться к адаптеру Fast Ethernet под номером inum в списке адаптеров
    void disconnectAdapter();               // Отключиться от адаптера Fast Ethernet
    //---------------------------------------------------------------------

    void messageProccess(const Message& msg); // Обработчик принятых MAC пакетов

public slots:
    void start();
    void checkReceived(); // Проверка сокета на наличие принятых MAC пакетов

signals:
    void macTimerReceiveRestart(); // Запуск таймера вызова обработчика принятых MAC пакетов

private:
    TimeProvider* m_timeProvider;
};

#endif // ETHHANDLER_H
