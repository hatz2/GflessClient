#include "editmultipleprofileaccountsdialog.h"
#include "qdebug.h"
#include "ui_editmultipleprofileaccountsdialog.h"

bool EditMultipleProfileAccountsDialog::defaultAutoLogin = false;
int EditMultipleProfileAccountsDialog::defaultServerLocation = 0;
int EditMultipleProfileAccountsDialog::defaultServer = 0;
int EditMultipleProfileAccountsDialog::defaultChannel = 0;
int EditMultipleProfileAccountsDialog::defaultCharacter = 0;

EditMultipleProfileAccountsDialog::EditMultipleProfileAccountsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditMultipleProfileAccountsDialog)
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    ui->loginCheckBox->setChecked(defaultAutoLogin);
    ui->serverLocationComboBox->setCurrentIndex(defaultServerLocation);
    ui->serverComboBox->setCurrentIndex(defaultServer);
    ui->channelComboBox->setCurrentIndex(defaultChannel);
    ui->characterComboBox->setCurrentIndex(defaultCharacter);
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

    defaultAutoLogin = autoLogin;
    defaultServerLocation = serverLocation;
    defaultServer = server;
    defaultChannel = channel;
    defaultCharacter = character;

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

