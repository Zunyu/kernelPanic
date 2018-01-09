#include "checklocthread.h"
#include <QDebug>
#include <QFile>
///This is a thread for checking the obstacle file every 10 seconds

CheckLocThread::CheckLocThread()
{

}
void CheckLocThread::run(){

    while(1)
    {
        QFile data("Location.txt");
        if(data.open(QFile::ReadOnly)){
        QString str = "Obstacle Encountered";
        emit signal_add_int(str);
        sleep(10);
        qDebug()<<"run thread:" <<QThread::currentThreadId();
        }
    }
}
