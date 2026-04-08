#ifndef EDITPROXYDIALOG_H
#define EDITPROXYDIALOG_H

#include <QCheckBox>
#include <QDialog>
#include <QLineEdit>

class EditProxyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditProxyDialog(QWidget* parent = nullptr);

    void setValues(
        bool useProxyValue,
        const QString& ipValue,
        const QString& portValue,
        const QString& usernameValue,
        const QString& passwordValue,
        const QString& identityPathValue,
        const QString& customGamePathValue,
        const QString& installationIdValue
    );

    bool getUseProxy() const;
    QString getProxyIp() const;
    QString getSocksPort() const;
    QString getProxyUsername() const;
    QString getProxyPassword() const;
    QString getIdentityPath() const;
    QString getCustomGamePath() const;
    QString getInstallationId() const;

protected:
    void accept() override;

private:
    void updateProxyFieldsEnabled(bool enabled);

    QCheckBox* useProxyCheckBox;
    QLineEdit* proxyIpLineEdit;
    QLineEdit* socksPortLineEdit;
    QLineEdit* proxyUsernameLineEdit;
    QLineEdit* proxyPasswordLineEdit;
    QLineEdit* identityPathLineEdit;
    QLineEdit* customGamePathLineEdit;
    QLineEdit* installationIdLineEdit;
};

#endif // EDITPROXYDIALOG_H
