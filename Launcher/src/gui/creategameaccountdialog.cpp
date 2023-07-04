#include "creategameaccountdialog.h"
#include "ui_creategameaccountdialog.h"
#include <QMessageBox>

CreateGameAccountDialog::CreateGameAccountDialog(const QVector<GameforgeAccount *> &gfAccs, const int gfLangIndex, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CreateGameAccountDialog),
    gfAccounts(gfAccs)
{
    ui->setupUi(this);

    for (const auto& gfAcc : gfAccounts) {
        ui->gfAccountComboBox->addItem(gfAcc->getEmail());
    }

    switch (gfLangIndex) {
    case 0:
        gfLang = "en";
        break;
    case 1:
        gfLang = "de";
        break;
    case 2:
        gfLang = "fr";
        break;
    case 3:
        gfLang = "it";
        break;
    case 4:
        gfLang = "pl";
        break;
    case 5:
        gfLang = "es";
        break;
    case 7:
        gfLang = "cz";
        break;
    case 8:
        gfLang = "tr";
        break;
    default:
        gfLang = "en";
        break;
    }
}

CreateGameAccountDialog::~CreateGameAccountDialog()
{
    delete ui;
}

void CreateGameAccountDialog::on_createAccButton_clicked()
{
    int index = ui->gfAccountComboBox->currentIndex();

    if (index < 0)
        return;

    QString nameAccount = ui->accountNameLineEdit->text();
    gfAcc = gfAccounts[index];

    QJsonObject response;

    if (!gfAcc->createGameAccount(nameAccount, gfLang, response)) {
        QMessageBox::critical(this, "Error", response["error"].toObject()["message"].toString());
        return;
    }

    newAccountInfo = response;

    QMessageBox::information(this, "Error", "Account created successfully. Account name: " + newAccountInfo["displayName"].toString());
    accept();
}

GameforgeAccount *CreateGameAccountDialog::getGfAcc() const
{
    return gfAcc;
}

QJsonObject CreateGameAccountDialog::getNewAccountInfo() const
{
    return newAccountInfo;
}

