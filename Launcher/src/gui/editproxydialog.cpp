#include "editproxydialog.h"

#include <QDir>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

EditProxyDialog::EditProxyDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Edit account settings");
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    useProxyCheckBox = new QCheckBox("Use SOCKS5 proxy", this);
    proxyIpLineEdit = new QLineEdit(this);
    socksPortLineEdit = new QLineEdit(this);
    proxyUsernameLineEdit = new QLineEdit(this);
    proxyPasswordLineEdit = new QLineEdit(this);
    identityPathLineEdit = new QLineEdit(this);
    customGamePathLineEdit = new QLineEdit(this);
    installationIdLineEdit = new QLineEdit(this);

    proxyPasswordLineEdit->setEchoMode(QLineEdit::Password);

    QFormLayout* formLayout = new QFormLayout();
    formLayout->addRow(useProxyCheckBox);
    formLayout->addRow("Proxy IP:", proxyIpLineEdit);
    formLayout->addRow("Port:", socksPortLineEdit);
    formLayout->addRow("Username:", proxyUsernameLineEdit);
    formLayout->addRow("Password:", proxyPasswordLineEdit);
    QWidget* customGamePathWidget = new QWidget(this);
    QHBoxLayout* customGamePathLayout = new QHBoxLayout(customGamePathWidget);
    customGamePathLayout->setContentsMargins(0, 0, 0, 0);
    customGamePathLayout->setSpacing(6);
    QPushButton* browseCustomGamePathButton = new QPushButton("...", customGamePathWidget);
    browseCustomGamePathButton->setToolTip("Select custom game client (.exe)");
    browseCustomGamePathButton->setFixedWidth(32);
    customGamePathLayout->addWidget(customGamePathLineEdit);
    customGamePathLayout->addWidget(browseCustomGamePathButton);

    formLayout->addRow("Identity path (optional):", identityPathLineEdit);
    formLayout->addRow("Custom game path (optional):", customGamePathWidget);
    formLayout->addRow("Custom installation id (optional):", installationIdLineEdit);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal,
        this
    );

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    connect(useProxyCheckBox, &QCheckBox::toggled, this, &EditProxyDialog::updateProxyFieldsEnabled);
    connect(browseCustomGamePathButton, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(
            this,
            "Select custom game client",
            QDir::rootPath(),
            "(*.exe)"
        );

        if (!path.isEmpty()) {
            customGamePathLineEdit->setText(path);
        }
    });
    connect(buttonBox, &QDialogButtonBox::accepted, this, &EditProxyDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &EditProxyDialog::reject);

    updateProxyFieldsEnabled(false);
}

void EditProxyDialog::setValues(
    bool useProxyValue,
    const QString& ipValue,
    const QString& portValue,
    const QString& usernameValue,
    const QString& passwordValue,
    const QString& identityPathValue,
    const QString& customGamePathValue,
    const QString& installationIdValue
)
{
    useProxyCheckBox->setChecked(useProxyValue);
    proxyIpLineEdit->setText(ipValue);
    socksPortLineEdit->setText(portValue);
    proxyUsernameLineEdit->setText(usernameValue);
    proxyPasswordLineEdit->setText(passwordValue);
    identityPathLineEdit->setText(identityPathValue);
    customGamePathLineEdit->setText(customGamePathValue);
    installationIdLineEdit->setText(installationIdValue);
    updateProxyFieldsEnabled(useProxyValue);
}

bool EditProxyDialog::getUseProxy() const
{
    return useProxyCheckBox->isChecked();
}

QString EditProxyDialog::getProxyIp() const
{
    return proxyIpLineEdit->text();
}

QString EditProxyDialog::getSocksPort() const
{
    return socksPortLineEdit->text();
}

QString EditProxyDialog::getProxyUsername() const
{
    return proxyUsernameLineEdit->text();
}

QString EditProxyDialog::getProxyPassword() const
{
    return proxyPasswordLineEdit->text();
}

QString EditProxyDialog::getIdentityPath() const
{
    return identityPathLineEdit->text();
}

QString EditProxyDialog::getCustomGamePath() const
{
    return customGamePathLineEdit->text();
}

QString EditProxyDialog::getInstallationId() const
{
    return installationIdLineEdit->text();
}

void EditProxyDialog::accept()
{
    if (getUseProxy() && (getProxyIp().trimmed().isEmpty() || getSocksPort().trimmed().isEmpty())) {
        QMessageBox::critical(this, "Error", "If proxy is enabled, IP and port are required.");
        return;
    }

    QDialog::accept();
}

void EditProxyDialog::updateProxyFieldsEnabled(bool enabled)
{
    proxyIpLineEdit->setEnabled(enabled);
    socksPortLineEdit->setEnabled(enabled);
    proxyUsernameLineEdit->setEnabled(enabled);
    proxyPasswordLineEdit->setEnabled(enabled);
}
