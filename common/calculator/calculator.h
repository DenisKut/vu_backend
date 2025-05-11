#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <QObject>

class Calculator : public QObject {
    Q_OBJECT
public:
    Calculator();

    struct Point {

    };

    // Параметры ЦУ
    struct paramCU {
        int64_t time;   // Время получения замера (системное)

        double az;      // Азимут
        double an;      // Угол места
        double r;       // Дальность

        double azV;     // Скорость изменения азимута
        double anV;     // Скорость изменения угла места
        double rV;      // Скорость изменения дальности

        double azEr;    // Ошибка по азимуту
        double anEr;    // Ошибка по углу места
        double rEr;     // Ошибка по дальности

        // Информационные параметры (тип цели и т.д.)
    };

    // Параметры ППУ
    struct paramPPU {
        int number;     // Номер ППУ
        int64_t time;   // Время обновления координат от ППУ (системное)

        paramCU m_paramCU;
    };

//    void getPoint(int64_t timePrev, int64_t timeNew
//                  double);

};

#endif // CALCULATOR_H
