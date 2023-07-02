#include "editmultipleprofileaccountsdialog.h"
#include "ui_editmultipleprofileaccountsdialog.h"

EditMultipleProfileAccountsDialog::EditMultipleProfileAccountsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditMultipleProfileAccountsDialog)
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
}

EditMultipleProfileAccountsDialog::EditMultipleProfileAccountsDialog(bool login, int serverLoc, int serverIndex, int channelIndex, int characterIndex, QWidget *parent)
    : EditMultipleProfileAccountsDialog(parent)
{
    ui->loginCheckBox->setChecked(login);
    ui->serverLocationComboBox->setCurrentIndex(serverLoc);
    ui->serverComboBox->setCurrentIndex(serverIndex);
    ui->channelComboBox->setCurrentIndex(channelIndex);
    ui->characterComboBox->setCurrentIndex(characterIndex);
}

EditMultipleProfileAccountsDialog::~EditMultipleProfileAccountsDialog()
{
    delete ui;
}

void EditMultipleProfileAccountsDialog::on_loginCheckBox_stateChanged(int arg1)
{
    ui->serverLocationComboBox->setEnabled(arg1);
    ui->serverLocationLabel->setEnabled(arg1);
    ui->serverComboBox->setEnabled(arg1);
    ui->serverLabel->setEnabled(arg1);
    ui->channelComboBox->setEnabled(arg1);
    ui->channelLabel->setEnabled(arg1);
    ui->characterComboBox->setEnabled(arg1);
    ui->characterLabel->setEnabled(arg1);
}


void EditMultipleProfileAccountsDialog::on_addProfAccountButton_clicked()
{
    autoLogin = ui->loginCheckBox->isChecked();
    serverLocation = ui->serverLocationComboBox->currentIndex();
    server = ui->serverComboBox->currentIndex();
    channel = ui->channelComboBox->currentIndex();
    character = ui->characterComboBox->currentIndex();

    accept();
}

bool EditMultipleProfileAccountsDialog::getAutoLogin() const
{
    return autoLogin;
}

int EditMultipleProfileAccountsDialog::getServerLocation() const
{
    return serverLocation;
}

int EditMultipleProfileAccountsDialog::getServer() const
{
    return server;
}

int EditMultipleProfileAccountsDialog::getChannel() const
{
    return channel;
}

int EditMultipleProfileAccountsDialog::getCharacter() const
{
    return character;
}

