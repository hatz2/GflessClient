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

void SettingsDialog::on_selectGamePathButton_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, "Select nostale client", QDir::rootPath(), "Nostale client (NostaleClientX.exe)");

    if (path.isEmpty())
        return;

    setGameClientPath(path);
}

