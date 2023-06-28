#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QFileDialog>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    QString getGameClientPath() const;

    QString getProfilesPath() const;

    QString getIdentityPath() const;

    int getOpenInterval() const;

    int getGameLanguage() const;

    int getGameLanguageIndex() const;

    int getServerLanguage() const;

    int getServer() const;

    bool autoLogIn() const;

    int getTheme() const;

    void setGameClientPath(const QString& path);

    void setProfilesPath(const QString& path);

    void setIdentityPath(const QString& path);

    void setOpenInterval(int n);

    void setGameLanguage(int language);

    void setServerLanguage(int servLang);

    void setServer(int server);

    void setAutoLogin(bool login);

    void setTheme(int index);

    void setThemeComboBox(int index);

signals:
    void autoLoginStateChanged(bool);

    void profilesPathSelected(QString);

    void identityPathSelected(QString);

private slots:
    void on_selectGamePathButton_clicked();

    void on_autoLogInCheckBox_stateChanged(int arg1);

    void on_selectProfilePathButton_clicked();

    void on_selectIdentityButton_clicked();

    void on_themeComboBox_currentIndexChanged(int index);

private:
    void initLanguageComboBox();

    Ui::SettingsDialog *ui;
};


#endif // SETTINGSDIALOG_H
