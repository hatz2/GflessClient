#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "NostaleAuth.h"
#include "GflessClient.h"
#include "AddAccountDialog.h"
#include "SettingsDialog.h"
#include "AccountProfile.h"
#include "AddProfileDialog.h"
#include "AddProfileAccountDialog.h"

#include <QMainWindow>
#include <QTimer>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QCloseEvent>
#include <QList>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void on_openAccountsButton_clicked();

    void on_addGameforgeAccountButton_clicked();

    void on_gameforgeAccountComboBox_currentTextChanged(const QString &arg1);

    void on_removeGameforgeAccountButton_clicked();

    void on_gameSettingsButton_clicked();

    void on_actionSettings_triggered();

    void on_actionAbout_3_triggered();

    void on_actionAbout_Qt_triggered();

    void iconActivated(QSystemTrayIcon::ActivationReason reason);

    void on_actionGet_help_triggered();

    void handleLocalConnection();

    void on_addProfileButton_clicked();

    void on_profileComboBox_currentIndexChanged(const QString &arg1);

    void on_addProfileAccountButton_clicked();

private:
    void createTrayIcon();

    bool checkGameClientPath();

    void loadSettings();

    void saveSettings();

    void displayGameAccounts(const QString& gameforgeAccount, const QString &profileName = QString());

    void displayProfiles(const QString& gameforgeAccount);

    void addGameforgeAccount(const QString& email, const QString& password);

    Ui::MainWindow *ui;
    SettingsDialog* settingsDialog;
    GflessClient* gflessClient;
    QMap<QString /* gameforge account name */, QMap<QString /* display name */, QString /* id */>> accounts;
    QMap<QString /* gameforge account name */, NostaleAuth*> gameforgeAccounts;
//    QMap<QString /* gameforge account name */, QMap<QString /* profile name */, AccountProfile>> profiles;

//    QMap<QString /* gameforge account name */, QMap<QString /* profile name*/, QMap<QString /* fake name */, QPair<QString /* real name */, QString /* id */>>>> profiles;

    //QList <AccountProfile> profiles;
    QMap <QString /* profile name */, AccountProfile> profiles;

    QSystemTrayIcon* trayIcon;
    QLocalServer* gflessServer;
};
#endif // MAINWINDOW_H
