#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    initLanguageComboBox();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

QString SettingsDialog::getGameClientPath() const
{
    return ui->gameClientPathLineEdit->text();
}

QString SettingsDialog::getProfilesPath() const
{
    return ui->profilesPathLineEdit->text();
}

QString SettingsDialog::getIdentityPath() const
{
    return ui->identityLineEdit->text();
}

int SettingsDialog::getOpenInterval() const
{
    return ui->openIntervalSpinBox->value();
}

int SettingsDialog::getGameLanguage() const
{
    return ui->gameLanguageComboBox->currentData().toInt();
}

int SettingsDialog::getGameLanguageIndex() const
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

bool SettingsDialog::autoLogIn() const
{
    return ui->autoLogInCheckBox->isChecked();
}

int SettingsDialog::getTheme() const
{
    return ui->themeComboBox->currentIndex();
}

void SettingsDialog::setGameClientPath(const QString &path)
{
    ui->gameClientPathLineEdit->setText(path);
}

void SettingsDialog::setProfilesPath(const QString &path)
{
    ui->profilesPathLineEdit->setText(path);
}

void SettingsDialog::setIdentityPath(const QString &path)
{
    ui->identityLineEdit->setText(path);
    emit identityPathSelected(path);
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

void SettingsDialog::setAutoLogin(bool login)
{
    ui->autoLogInCheckBox->setChecked(login);
}

void SettingsDialog::setTheme(int index)
{
    switch (index) {
    case 0:
        QApplication::setStyle("Fusion");
        break;
    case 1:
        QApplication::setStyle("Windowsvista");
        break;
    default:
        break;
    }
}

void SettingsDialog::setThemeComboBox(int index)
{
    ui->themeComboBox->setCurrentIndex(index);
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
    ui->serverLanguageComboBox->setEnabled(arg1);
    ui->serverComboBox->setEnabled(arg1);

    emit autoLoginStateChanged(arg1);
}

void SettingsDialog::initLanguageComboBox()
{
    ui->gameLanguageComboBox->addItem("English", 0);
    ui->gameLanguageComboBox->addItem("Deutsch", 1);
    ui->gameLanguageComboBox->addItem("Français", 2);
    ui->gameLanguageComboBox->addItem("Italiano", 3);
    ui->gameLanguageComboBox->addItem("Polski", 4);
    ui->gameLanguageComboBox->addItem("Español", 5);
    ui->gameLanguageComboBox->addItem("Türkçe", 8);
    ui->gameLanguageComboBox->addItem("Čeština", 7);
}


void SettingsDialog::on_selectProfilePathButton_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, "Select profile", QDir::rootPath(), "(*.ini)");

    if (path.isEmpty())
        return;

    setProfilesPath(path);

    emit profilesPathSelected(path);
}


void SettingsDialog::on_selectIdentityButton_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, "Select identity file", QDir::rootPath(), "(*.json)");

    if (path.isEmpty())
        return;

    setIdentityPath(path);
}



void SettingsDialog::on_themeComboBox_currentIndexChanged(int index)
{
    setTheme(index);
}

