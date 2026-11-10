#include "identitydialog.h"
#include "ui_identitydialog.h"
#include "identityrandomizer.h"

#include <QJsonDocument>
#include <QJsonObject>


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

    // Re-roll every machine-stable fingerprint field so each generated file is a
    // distinct device (uuid, memory, cores, screen, gpu, profile hashes...).
    // Only locale-descriptive fields (tz, lang, os, user agent) are kept.
    QJsonObject identityJson = QJsonDocument::fromJson(identity).object();
    QJsonObject uniqueIdentity = IdentityRandomizer::randomize(identityJson);

    QString path = QFileDialog::getSaveFileName(this, "Save identity", QDir::rootPath(), "(*.json)");

    if (path.isEmpty())
        return;

    QFile file(path);

    if (!file.open(QFile::WriteOnly | QFile::Truncate))
    {
        QMessageBox::critical(this, "Error", "Can't open the selected file");
        return;
    }

    file.write(QJsonDocument(uniqueIdentity).toJson());

    file.close();

    QMessageBox::information(this, "Success",
        "Identity file was generated successfully.\n\n"
        "All device-identifying fields were randomized,\n"
        "so every identity you generate is unique.");

    accept();
}

