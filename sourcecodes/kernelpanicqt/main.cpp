#include "mainwindow.h"
#include <QApplication>
#include "locationdlg.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    locationDlg loc1;
    if(loc1.exec()==QDialog::Accepted){
        w.show();//show the mainwindow(loop)
        return a.exec();
    }

    else return 0;
}

