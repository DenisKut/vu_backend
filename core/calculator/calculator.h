#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <QObject>

class Calculator : public QObject {
    Q_OBJECT
public:
    Calculator();

    // Целеуказание
    struct newCU {
        int numPPU; // Номер ППУ
        int64_t time; // Время прихода ЦУ (системное)

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

};

#endif // CALCULATOR_H
