#include "locationdlg.h"
#include "ui_locationdlg.h"

locationDlg::locationDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::locationDlg)
{
    ui->setupUi(this);
}

locationDlg::~locationDlg()
{
    delete ui;
}
