#include "addaccountdialog.h"
#include "ui_addaccountdialog.h"
#include <QFileDialog>
#include <QUuid>

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
    useProxy = ui->proxyCheckBox->isChecked();
    proxyIp = ui->proxyIpLineEdit->text();
    socksPort = ui->socksPortLineEdit->text();
    proxyUsername = ui->proxyUsernameLineEdit->text();
    proxyPassword = ui->proxyPasswordLineEdit->text();
    customClientPath = ui->customGamePathLineEdit->text();
    installationId = ui->installationIdLineEdit->text();

    if (email.isEmpty() || password.isEmpty() || identityPath.isEmpty())
    {
        QMessageBox::critical(this, "Error", "Email, password or identity path field is empty");
        return;
    }

    if (useProxy && (proxyIp.isEmpty() || socksPort.isEmpty())) {
        QMessageBox::critical(this, "Error", "Proxy IP or Port is empty");
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


void AddAccountDialog::on_proxyCheckBox_stateChanged(int arg1)
{
    ui->proxyIpLabel->setEnabled(arg1);
    ui->proxyIpLineEdit->setEnabled(arg1);
    ui->socksPortLabel->setEnabled(arg1);
    ui->socksPortLineEdit->setEnabled(arg1);
    ui->proxyUsernameLineEdit->setEnabled(arg1);
    ui->proxyUsernameLabel->setEnabled(arg1);
    ui->proxyPasswordLineEdit->setEnabled(arg1);
    ui->proxyPasswordLabel->setEnabled(arg1);
}

QString AddAccountDialog::getProxyPassword() const
{
    return proxyPassword;
}

QString AddAccountDialog::getProxyUsername() const
{
    return proxyUsername;
}

QString AddAccountDialog::getProxyIp() const
{
    return proxyIp;
}

QString AddAccountDialog::getSocksPort() const
{
    return socksPort;
}

bool AddAccountDialog::getUseProxy() const
{
    return useProxy;
}


void AddAccountDialog::on_removeCustomGamePathButton_clicked()
{
    ui->customGamePathLineEdit->setText(QString());
}


void AddAccountDialog::on_selectCustomGamePathButton_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, "Select custom game client", QDir::rootPath(), "(*.exe)");

    if (path.isEmpty())
        return;

    ui->customGamePathLineEdit->setText(path);
}

QString AddAccountDialog::getCustomClientPath() const
{
    return customClientPath;
}


void AddAccountDialog::on_installationIdButton_clicked()
{
    ui->installationIdLineEdit->setText(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

QString AddAccountDialog::getInstallationId() const
{
    return installationId;
}

