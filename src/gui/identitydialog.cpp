#include "identitydialog.h"
#include "ui_identitydialog.h"

IdentityDialog::IdentityDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IdentityDialog)
{
    ui->setupUi(this);
}

IdentityDialog::~IdentityDialog()
{
    delete ui;
}

void IdentityDialog::on_generateButton_clicked()
{
    QString blackbox = ui->blackboxTextEdit->toPlainText();

    if (blackbox.isEmpty())
    {
        QMessageBox::critical(this, "Error", "Blackbox can't be empty");
        return;
    }

    QByteArray identity = BlackBox::decode(blackbox.toLocal8Bit());

    if (identity.isEmpty())
    {
        QMessageBox::critical(this, "Error", "Error decoding blackbox");
        return;
    }

    QString path = QFileDialog::getSaveFileName(this, "Save identity", QDir::rootPath(), "(*.json)");

    if (path.isEmpty())
        return;

    QFile file(path);

    if (!file.open(QFile::WriteOnly | QFile::Truncate))
    {
        QMessageBox::critical(this, "Error", "Can't open the selected file");
        return;
    }

    file.write(identity);

    file.close();

    QMessageBox::information(this, "Success", "Identity file was generated successfully");

    accept();
}

