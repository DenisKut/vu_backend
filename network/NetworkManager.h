#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QTimer>
#include <QDebug>
#include "handlers/TcpHandler.h"
#include "handlers/UdpHandler.h"
#include "handlers/EthHandler.h"
#include "utils/time_provider/TimeProvider.h"
#include "utils/AppConfig.h"
#include <math.h>

class NetworkManager : public QObject {
    Q_OBJECT
public:
    explicit NetworkManager(TimeProvider* timeProvider, QObject* parent = nullptr);
    ~NetworkManager();

    void start();
    void stop();

    // ППУ:
    //----------------------------------------------------------
    void calculateCoefficients(double tPos, double tVel,
                               double cPos, double cVel, double dt,
                               double aMax, double vMax,
                               double* C0, double* C1, double* C2) {
        // 1. Расчет ошибки позиции и скорости
        float delta_x = tPos - cPos;
        float delta_v = tVel - cVel;

        // 2. Расчет идеального ускорения для достижения цели
        float a_needed = (delta_v + 2.0f * delta_x / dt) / dt;

        // 3. Ограничение ускорения по максимальному значению
        a_needed = fmaxf(fminf(a_needed, aMax), -aMax);

        // 4. Проверка ограничения скорости после ускорения
        float predicted_velocity = cVel + a_needed * dt;
        if (predicted_velocity > vMax) {
            a_needed = (vMax - cVel) / dt;
        } else if (predicted_velocity < -vMax) {
            a_needed = (-vMax - cVel) / dt;
        }

        // 5. Установка коэффициентов
        *C0 = 0.0f;              // Начальное смещение (не используется)
        *C1 = cVel;   // Текущая скорость (начальная для интервала)
        *C2 = 0.5f * a_needed;   // Ускорение, деленное на 2 (по условию задачи)
    }
    //----------------------------------------------------------


public slots:
    void onTcpReceived(const TcpHeader& msg);
    void onUdpReceived(const Message& msg);
    void onEthReceived(const QByteArray &msg);

private:
    TimeProvider* m_timeProvider;
    QTimer m_ethTimer;
    TcpHandler m_tcpHandler;
    UdpHandler m_udpHandler;
    EthHandler m_ethHandler;
};

#endif // NETWORKMANAGER_H
