#include "AddAccountDialog.h"
#include "ui_AddAccountDialog.h"

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

    if (email.isEmpty() || password.isEmpty())
    {
        QMessageBox::critical(this, "Error", "Email or password field is empty");
        return;
    }

    accept();
}

