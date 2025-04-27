#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QTimer>
#include <QDebug>
#include "handlers/TcpHandler.h"
#include "handlers/UdpHandler.h"
#include "handlers/EthHandler.h"
#include "utils/time_provider/TimeProvider.h"
#include "core/logger/Logger.h"
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
    void calculateCoefficientsPPU(double angleTarget,  double velocityTarget,
                                  double angleCurrent, double* angleLast,
                                  double* accelerationLast, double accelerationMax,
                                  double angleMin, double velocityMin,
                                  double KP, double KD, double dt,
                                  double* C0, double* C1, double* C2) {


        double dx = angleTarget - angleCurrent;
        double v1 = (angleCurrent - (*angleLast)) / dt + 0.5 * (*accelerationLast) * dt;
        double dv = velocityTarget - v1;
        double a1 = 0.0;

        double breaking_dist = dv * dv / (2.0 * accelerationMax) + angleMin;

        if (abs(dx) > angleMin) {
            // Максимальное ускорение в направлении цели
            a1 = (dx > 0) ? accelerationMax : -accelerationMax;

            // Если расстояние торможения достигнуто, то тормозим:
            if (abs(dx) < breaking_dist) {
                a1 = (dv < 0) ? -accelerationMax : accelerationMax;
            }

            // Плавное изменение ускорения:
            if (abs(dv) < velocityMin) {
                a1 = (*accelerationLast) + 0.5 * (a1 - (*accelerationLast));
            }
        } else {
            // ПД-регулятор
            a1 = KP * dx + KD * dv;
        }

        // Ограничение ускорения
        a1 = fmax(fmin(a1, accelerationMax), -accelerationMax);

        // Плавное изменение ускорения:
        if (abs(dv) < velocityMin) {
            a1 = (*accelerationLast) + 0.8 * (a1 - (*accelerationLast));
        }

        // Запоминаем предыдущие значения положения и ускорения точки 1
        *angleLast = angleCurrent;
        *accelerationLast = a1;

        // Установка коэффициентов
        *C0 = angleCurrent; // Начальное смещение (не используется)
        *C1 = v1;           // Текущая скорость (начальная для интервала)
        *C2 = 0.5 * a1;     // Ускорение, деленное на 2
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
