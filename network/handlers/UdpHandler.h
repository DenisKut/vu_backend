#pragma once

#include <QObject>
#include <QHostAddress>
#include <QUdpSocket>
#include <QPointer>
#include <atomic>
#include <thread>
#include "utils/time_provider/TimeProvider.h"
#include "../messages/UdpMessage.h"

class UdpHandler : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(UdpHandler)
public:
    /**
     * @brief Конструктор обработчика UDP
     * @param parent Родительский QObject
     */
    explicit UdpHandler(QObject* parent = nullptr);
    ~UdpHandler() override;

    /**
     * @brief Запустить UDP-сокет на указанном IP и порту
     */
    Q_INVOKABLE void start(const QString& ip, int port);
    /**
     * @brief Остановить UDP-сокет
     */
    void stop();

    /**
     * @brief Разобрать входящий пакет (вызывается из onReadyRead)
     */
    Q_INVOKABLE void processIncomingDatagram(const QByteArray& data,
                                 const QHostAddress& sender,
                                 quint16 senderPort);

    /**
     * @brief Сформировать и отправить кастомный пакет
     */
    Q_INVOKABLE void sendCustomPacket(const QByteArray& payload,
                                const QHostAddress& addr,
                                quint16 port);

signals:
    /**
     * @brief Сигнал о приёме UDP-пакета
     */
    void packetReceived(const QByteArray& data,
                        const QHostAddress& sender,
                        quint16 senderPort);
    // ---
    // Пример потокобезопасного использования:
    // // В вашем классе:
    // signals:
    //     void needSendUdp(const QByteArray& data, const QHostAddress& addr, quint16 port);
    // // В инициализации (например, в NetworkManager):
    // connect(yourObject, &YourClass::needSendUdp,
    //         udpHandler, &UdpHandler::sendMessage,
    //         Qt::QueuedConnection);
    // // Для получения:
    // connect(udpHandler, &UdpHandler::packetReceived,
    //         yourObject, &YourClass::onUdpPacketReceived,
    //         Qt::QueuedConnection);
    // // В вашем классе реализуйте слот:
    // public slots:
    //     void onUdpPacketReceived(const QByteArray& data, const QHostAddress& sender, quint16 senderPort);
    // ---

public slots:
    /**
     * @brief Отправить UDP-пакет на фиксированный адрес
     */
    Q_INVOKABLE void sendMessage(const QByteArray& data);

    /**
     * @brief Тестовая отправка пакетов на МЦФОС
     */
    void testMCFOS();

private slots:
    void onReadyRead();

private:
    quint16 m_port = 0;
    QPointer<QUdpSocket> m_socket;
    QHostAddress m_targetAddr;
    quint16 m_targetPort = 0;

    QVector<quint32> vecSignalDAC_Re, vecSignalDAC_Im; // numBuf: 1, 2
    QVector<quint32> vecSignalREF_Re, vecSignalREF_Im; // numBuf: 3, 4
    void writeSignalToMCFOS(UdpMessage_MCFOS_Cmd *mes,
                            QVector<quint32> vecSignalDAC[]);

    quint8 m_mode = 0; ///< Режим работы МЦФОС
    /// 0 - ожидание
    /// 1 - очистка буферов ЦАП и Ref
    /// 2 - запись сигнала в буфер ЦАП
    /// 3 - запись сигнала в буфер REF
    /// 4 - излучение и приём
    /// 5 - проверка на наличие ошибок записи результата в DDR
    /// 6 - чтение результата из DDR

    quint8 m_step = 0; ///< Текущий шаг взаимодействия с МЦФОС
                       ///< в текущем режиме работы
};
