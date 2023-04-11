#ifndef ADDACCOUNTDIALOG_H
#define ADDACCOUNTDIALOG_H

#include <QDialog>
#include <QMessageBox>

namespace Ui {
class AddAccountDialog;
}

class AddAccountDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddAccountDialog(QWidget *parent = nullptr);
    ~AddAccountDialog();

    const QString &getEmail() const;

    const QString &getPassword() const;

private slots:
    void on_showPasswordCheckBox_stateChanged(int arg1);

    void on_loginButton_clicked();

private:
    Ui::AddAccountDialog *ui;
    QString email;
    QString password;
};

#endif // ADDACCOUNTDIALOG_H
