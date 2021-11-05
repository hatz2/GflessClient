#include "addprofiledialog.h"
#include "ui_addprofiledialog.h"

AddProfileDialog::AddProfileDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddProfileDialog)
{
    ui->setupUi(this);

    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
}

AddProfileDialog::~AddProfileDialog()
{
    delete ui;
}

const QString &AddProfileDialog::getProfileName() const
{
    return profileName;
}

void AddProfileDialog::on_createProfileButton_clicked()
{
    profileName = ui->profileNameLineEdit->text();

    if (profileName.isEmpty())
    {
        QMessageBox::critical(this, "Error", "Profile name cannot be empty");
        return;
    }

    accept();
}

