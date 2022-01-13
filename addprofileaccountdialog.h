#ifndef ADDPROFILEACCOUNTDIALOG_H
#define ADDPROFILEACCOUNTDIALOG_H

#include <QDialog>
#include <QMessageBox>
#include <QStringList>

namespace Ui {
class AddProfileAccountDialog;
}

class AddProfileAccountDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddProfileAccountDialog(QStringList accounts, QWidget *parent = nullptr);
    ~AddProfileAccountDialog();

    const QString &getAccountName() const;

    const QString &getPseudonym() const;

private slots:
    void on_addProfAccountButton_clicked();

private:
    Ui::AddProfileAccountDialog *ui;
    QString accountName;
    QString pseudonym;
};

#endif // ADDPROFILEACCOUNTDIALOG_H
