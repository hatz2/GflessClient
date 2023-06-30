#include "addaccountdialog.h"
#include "ui_addaccountdialog.h"
#include <QFileDialog>

AddAccountDialog::AddAccountDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddAccountDialog)
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
}

AddAccountDialog::~AddAccountDialog()
{
    delete ui;
}

void AddAccountDialog::on_showPasswordCheckBox_stateChanged(int arg1)
{
    if (arg1)
        ui->passwordLineEdit->setEchoMode(QLineEdit::Normal);

    else
        ui->passwordLineEdit->setEchoMode(QLineEdit::Password);
}

const QString &AddAccountDialog::getPassword() const
{
    return password;
}

const QString &AddAccountDialog::getEmail() const
{
    return email;
}


void AddAccountDialog::on_loginButton_clicked()
{
    email = ui->emailLineEdit->text();
    password = ui->passwordLineEdit->text();
    identityPath = ui->identityPathLineEdit->text();

    if (email.isEmpty() || password.isEmpty() || identityPath.isEmpty())
    {
        QMessageBox::critical(this, "Error", "Email, password or identity path field is empty");
        return;
    }

    accept();
}

QString AddAccountDialog::getIdentityPath() const
{
    return identityPath;
}


void AddAccountDialog::on_selectIdentityButton_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, "Select identity file", QDir::rootPath(), "(*.json)");

    if (path.isEmpty())
        return;

    ui->identityPathLineEdit->setText(path);
}

