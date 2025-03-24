#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QThread>

class Logger : public QObject
{
    Q_OBJECT
public:
    explicit Logger(QObject *parent = nullptr);

private:
    QThread m_logThread;

public slots:


signals:

};

#endif // LOGGER_H
