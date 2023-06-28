#ifndef IDENTITYDIALOG_H
#define IDENTITYDIALOG_H

#include "blackbox.h"

#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>

namespace Ui {
class IdentityDialog;
}

class IdentityDialog : public QDialog
{
    Q_OBJECT

public:
    explicit IdentityDialog(QWidget *parent = nullptr);
    ~IdentityDialog();

private slots:
    void on_generateButton_clicked();

private:
    Ui::IdentityDialog *ui;
};

#endif // IDENTITYDIALOG_H
