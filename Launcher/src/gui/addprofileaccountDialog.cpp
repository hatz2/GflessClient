#include "addprofileaccountdialog.h"
#include "ui_addprofileaccountdialog.h"

bool AddProfileAccountDialog::autoLoginDefault = false;
int AddProfileAccountDialog::serverLocationDefault = 0;
int AddProfileAccountDialog::serverDefault = 0;
int AddProfileAccountDialog::channelDefault = 0;
int AddProfileAccountDialog::characterDefault = 0;

AddProfileAccountDialog::AddProfileAccountDialog(const QVector<GameforgeAccount *> accounts, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddProfileAccountDialog)
{
    ui->setupUi(this);

    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    gfAccounts = accounts;
    autoLogin = false;
    serverLocation = 0;
    server = 0;
    channel = 0;
    character = 0;

    for (const auto& acc : gfAccounts)
        ui->gameforgeAccountComboBox->addItem(acc->getEmail());


    ui->loginCheckBox->setChecked(AddProfileAccountDialog::autoLoginDefault);
    ui->serverLocationComboBox->setCurrentIndex(AddProfileAccountDialog::serverLocationDefault);
    ui->serverComboBox->setCurrentIndex(AddProfileAccountDialog::serverDefault);
    ui->channelComboBox->setCurrentIndex(AddProfileAccountDialog::channelDefault);
    ui->characterComboBox->setCurrentIndex(AddProfileAccountDialog::characterDefault);
}

AddProfileAccountDialog::AddProfileAccountDialog(const QVector<GameforgeAccount *> accounts, const GameAccount &gameAccount, QWidget *parent)
    : AddProfileAccountDialog(accounts, parent)
{
    for (int i = 0; i < gfAccounts.size(); ++i) {
        if (gfAccounts[i]->getEmail() == gameAccount.getGfAcc()->getEmail()) {
            ui->gameforgeAccountComboBox->setCurrentIndex(i);
            break;
        }
    }

    for (int i = 0; i < ui->gameAccountComboBox->count(); ++i) {
        if (ui->gameAccountComboBox->itemData(i).toString() == gameAccount.getId()) {
            ui->gameAccountComboBox->setCurrentIndex(i);
            break;
        }
    }

    ui->pseudonymLineEdit->setText(gameAccount.getDisplayName());
    ui->loginCheckBox->setChecked(gameAccount.getAutoLogin());
    ui->serverLocationComboBox->setCurrentIndex(gameAccount.getServerLocation());
    ui->serverComboBox->setCurrentIndex(gameAccount.getServer());
    ui->channelComboBox->setCurrentIndex(gameAccount.getChannel());
    ui->characterComboBox->setCurrentIndex(gameAccount.getSlot());

    ui->addProfAccountButton->setText("Edit account");

    setWindowTitle("Edit profile account");
}

AddProfileAccountDialog::~AddProfileAccountDialog()
{
    delete ui;
}

void AddProfileAccountDialog::on_addProfAccountButton_clicked()
{
    if (ui->gameforgeAccountComboBox->currentIndex() < 0)
        return;

    gfAcc = gfAccounts[ui->gameforgeAccountComboBox->currentIndex()];
    accountName = ui->gameAccountComboBox->currentText();
    pseudonym = ui->pseudonymLineEdit->text();
    autoLogin = ui->loginCheckBox->isChecked();
    serverLocation = ui->serverLocationComboBox->currentIndex();
    server = ui->serverComboBox->currentIndex();
    channel = ui->channelComboBox->currentIndex();
    character = ui->characterComboBox->currentIndex();
    id = ui->gameAccountComboBox->currentData().toString();

    if (pseudonym.isEmpty())
        pseudonym = accountName;

    if (accountName.isEmpty())
    {
        QMessageBox::critical(this, "Error", "Game account is not valid");
        return;
    }

    AddProfileAccountDialog::autoLoginDefault = autoLogin;
    AddProfileAccountDialog::serverLocationDefault = serverLocation;
    AddProfileAccountDialog::serverDefault = server;
    AddProfileAccountDialog::channelDefault = channel;
    AddProfileAccountDialog::characterDefault = character;

    accept();
}

const QString &AddProfileAccountDialog::getPseudonym() const
{
    return pseudonym;
}

const QString &AddProfileAccountDialog::getAccountName() const
{
    return accountName;
}


void AddProfileAccountDialog::on_loginCheckBox_stateChanged(int arg1)
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


void AddProfileAccountDialog::on_gameforgeAccountComboBox_currentIndexChanged(int index)
{
    ui->gameAccountComboBox->clear();

    if (index < 0)
        return;

    GameforgeAccount* acc = gfAccounts[index];

    auto gameAccs = acc->getGameAccounts();

    for (auto it = gameAccs.begin(); it != gameAccs.end(); ++it) {
        ui->gameAccountComboBox->addItem(it.value(), it.key());
    }
}

int AddProfileAccountDialog::getServerLocation() const
{
    return serverLocation;
}

QString AddProfileAccountDialog::getId() const
{
    return id;
}

GameforgeAccount *AddProfileAccountDialog::getGfAcc() const
{
    return gfAcc;
}

int AddProfileAccountDialog::getCharacter() const
{
    return character;
}

int AddProfileAccountDialog::getChannel() const
{
    return channel;
}

int AddProfileAccountDialog::getServer() const
{
    return server;
}

bool AddProfileAccountDialog::getAutoLogin() const
{
    return autoLogin;
}

