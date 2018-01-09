#ifndef LOCATIONDLG_H
#define LOCATIONDLG_H

#include <QDialog>

namespace Ui {
class locationDlg;
}

class locationDlg : public QDialog
{
    Q_OBJECT

public:
    explicit locationDlg(QWidget *parent = 0);
    ~locationDlg();

private:
    Ui::locationDlg *ui;
};

#endif // LOCATIONDLG_H
