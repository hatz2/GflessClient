#include "addprofileaccountdialog.h"
#include "ui_addprofileaccountdialog.h"

AddProfileAccountDialog::AddProfileAccountDialog(QStringList accounts, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddProfileAccountDialog)
{
    ui->setupUi(this);

    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    for (const auto& acc : accounts)
        ui->gameAccountComboBox->addItem(acc);
}

AddProfileAccountDialog::~AddProfileAccountDialog()
{
    delete ui;
}

void AddProfileAccountDialog::on_addProfAccountButton_clicked()
{
    accountName = ui->gameAccountComboBox->currentText();
    pseudonym = ui->pseudonymLineEdit->text();

    if (pseudonym.isEmpty())
        pseudonym = accountName;

    if (accountName.isEmpty())
    {
        QMessageBox::critical(this, "Error", "Game account is not valid");
        return;
    }

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

