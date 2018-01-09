#ifndef CHECKLOCTHREAD_H
#define CHECKLOCTHREAD_H


#include <QThread>
#include <QDebug>
class CheckLocThread : public QThread
{
    Q_OBJECT
public:
    CheckLocThread();
protected:
    void run();

signals:
    void signal_add_int(QString);
};

#endif // CHECKLOCTHREAD_H
