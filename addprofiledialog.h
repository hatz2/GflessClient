#ifndef ADDPROFILEDIALOG_H
#define ADDPROFILEDIALOG_H

#include <QDialog>
#include <QMessageBox>

namespace Ui {
class AddProfileDialog;
}

class AddProfileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddProfileDialog(QWidget *parent = nullptr);
    ~AddProfileDialog();

    const QString &getProfileName() const;

private slots:
    void on_createProfileButton_clicked();

private:
    Ui::AddProfileDialog *ui;
    QString profileName;
};

#endif // ADDPROFILEDIALOG_H
