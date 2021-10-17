#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

QString SettingsDialog::getGameClientPath() const
{
    return ui->gameClientPathLineEdit->text();
}

int SettingsDialog::getOpenInterval() const
{
    return ui->openIntervalSpinBox->value();
}

int SettingsDialog::getGameLanguage() const
{
    return ui->gameLanguageComboBox->currentIndex();
}

int SettingsDialog::getServerLanguage() const
{
    return ui->serverLanguageComboBox->currentIndex();
}

int SettingsDialog::getServer() const
{
    return ui->serverComboBox->currentIndex();
}

int SettingsDialog::getChannel() const
{
    return ui->channelComboBox->currentIndex();
}

bool SettingsDialog::autoLogIn() const
{
    return ui->autoLogInCheckBox->isChecked();
}

void SettingsDialog::setGameClientPath(const QString &path)
{
    ui->gameClientPathLineEdit->setText(path);
}

void SettingsDialog::setOpenInterval(int n)
{
    ui->openIntervalSpinBox->setValue(n);
}

void SettingsDialog::setGameLanguage(int language)
{
    ui->gameLanguageComboBox->setCurrentIndex(language);
}

void SettingsDialog::setServerLanguage(int servLang)
{
    ui->serverLanguageComboBox->setCurrentIndex(servLang);
}

void SettingsDialog::setServer(int server)
{
    ui->serverComboBox->setCurrentIndex(server);
}

void SettingsDialog::setChannel(int channel)
{
    ui->channelComboBox->setCurrentIndex(channel);
}

void SettingsDialog::setAutoLogin(bool login)
{
    ui->autoLogInCheckBox->setChecked(login);
}

void SettingsDialog::on_selectGamePathButton_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, "Select NostaleClientX.exe", QDir::rootPath(), "NostaleClientX.exe (NostaleClientX.exe)");

    if (path.isEmpty())
        return;

    setGameClientPath(path);
}


void SettingsDialog::on_autoLogInCheckBox_stateChanged(int arg1)
{
    ui->serverLabel->setEnabled(arg1);
    ui->serverLocationLabel->setEnabled(arg1);
    ui->channelLabel->setEnabled(arg1);
    ui->serverLanguageComboBox->setEnabled(arg1);
    ui->serverComboBox->setEnabled(arg1);
    ui->channelComboBox->setEnabled(arg1);
}

