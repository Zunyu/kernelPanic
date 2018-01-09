#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDebug>

int count=0;
char position[65536];
char positions[65536];
int posx[256];
int posy[256];
int i=0;
int MouseCheck=0;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{


    ui->setupUi(this);
    resize(480,250);
    pix=QPixmap(480,220);
    pix.fill(Qt::white);
    connect(&thread,SIGNAL(signal_add_int(QString)),this,SLOT(ObstacleMsg(QString)),Qt::QueuedConnection);
    thread.start();
        qDebug()<<"MainWindow:" <<QThread::currentThreadId();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::paintEvent(QPaintEvent*){
    QPainter pp(&pix);
    QPen pen(Qt::blue,3,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin);
    pp.setPen(pen);
    pp.drawLine(lastPoint,endPoint);
    lastPoint=endPoint;
    QPainter painter(this);
    painter.drawPixmap(0,0,pix);

}
void MainWindow::mouseDoubleClickEvent(QMouseEvent*event){//useless when implementing with the EC535 device


    if(event->button()==Qt::LeftButton)
        lastPoint=event->pos();
        endPoint=event->pos();

}
void MainWindow::mousePressEvent(QMouseEvent*event){


    if(event->button()==Qt::LeftButton&&MouseCheck%2==0)
        lastPoint=event->pos();

    endPoint=event->pos();
    lastPoint=endPoint;

    MouseCheck++;

}
void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if(event->buttons()&Qt::LeftButton) {

        endPoint = event->pos();
        count=count+1;
        update(); //drawing
        if (count%4==0){
            i=i+1;
            posx[i]=event->x();
            posy[i]=event->y();

        }

      }
}
void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{

    if(event->button() == Qt::LeftButton){
        endPoint = event->pos();


        update();
      }
}

void MainWindow::on_inputbtn_clicked()//input the route and generate a txt file contains the position information
{
    pix.fill(Qt::white);
    QFile posdata("posdata.txt");
    QFile::remove("posdata.txt");

    int j;


    for(j=2;j<i;j++){

        sprintf(position,"%d,%d,",posx[j]-posx[j-1],posy[j]-posy[j-1]);

        //printf(position);
        strcat(positions,position);

    }


    i=0;

    //printf("%s,end",positions);


        if(!posdata.open(QIODevice::WriteOnly|QIODevice::Text)){
            posdata.resize(0);
            return;
        }

        posdata.resize(0);
        QTextStream out(&posdata);
        out<<"R"<<"0,0,"<<positions<<"F";
        //printf(positions);
        posdata.close();
        pix.fill(Qt::white);
        memset(positions,0,1028);
        //loc2.show();

}

void MainWindow::on_getposbtn_clicked()//cancel button
{

    QFile::remove("posdata.txt");
    pix.fill(Qt::white);



    QMessageBox msgBox;
    msgBox.setText(tr("Input Canceled"));

   // msgBox.setInformativeText(tr("Cancel"));
    //msgBox.setDetailedText(tr(""));
    msgBox.setStandardButtons( QMessageBox::Ok);

    //msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();
    switch (ret) {

        case QMessageBox::Cancel:
            //qDebug() << "Close";
            break;
    }

}
void MainWindow::ObstacleMsg(QString str)//pop up a message box when obstacle encountered
{
    QMessageBox obsmsgBox;
    obsmsgBox.setText(str);
    obsmsgBox.setStandardButtons( QMessageBox::Cancel);
    obsmsgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = obsmsgBox.exec();
    switch (ret) {

        case QMessageBox::Cancel:
            qDebug() << "Close";
            break;
    }
    QFile::remove("Location.txt");

    qDebug()<<"from thread slot:" <<QThread::currentThreadId();
}
