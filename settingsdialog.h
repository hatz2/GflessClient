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

    int getOpenInterval() const;

    int getGameLanguage() const;

    int getGameLanguageIndex() const;

    int getServerLanguage() const;

    int getServer() const;

    bool autoLogIn() const;

    void setGameClientPath(const QString& path);

    void setOpenInterval(int n);

    void setGameLanguage(int language);

    void setServerLanguage(int servLang);

    void setServer(int server);

    void setAutoLogin(bool login);

signals:
    void autoLoginStateChanged(bool);

private slots:
    void on_selectGamePathButton_clicked();

    void on_autoLogInCheckBox_stateChanged(int arg1);

private:
    void initLanguageComboBox();

    Ui::SettingsDialog *ui;
};


#endif // SETTINGSDIALOG_H
