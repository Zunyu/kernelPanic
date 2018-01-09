#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMouseEvent>
#include <QPushButton>
#include "locationdlg.h"
#include <QMessageBox>
#include "checklocthread.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    //void pop();
    ~MainWindow();
protected:
    void paintEvent(QPaintEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void mouseDoubleClickEvent(QMouseEvent*);


private slots:
    void on_inputbtn_clicked();
    void on_getposbtn_clicked();
    void ObstacleMsg(QString);



private:
    Ui::MainWindow *ui;
    QPixmap pix;
    QPoint lastPoint;
    QPoint endPoint;
    locationDlg loc2;
    QMessageBox msgBox;
    CheckLocThread thread;
signals:
    //void ObstacleExist;

};

#endif // MAINWINDOW_H
