#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "gflessclient.h"
#include "settingsdialog.h"
#include "account.h"

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
#include <QMenu>
#include <QAction>
#include <QFile>
#include <QTextStream>


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

    void on_profileComboBox_currentIndexChanged(int index);

    void showContextMenu(const QPoint &pos);

    void on_removeProfileButton_clicked();

    void on_actionSave_profiles_triggered();

    void on_actionIdentity_generator_triggered();

private:
    void createTrayIcon();
    bool checkGameClientPath();
    bool checkIdentityPath();
    void loadSettings();
    void saveSettings();
    void loadAccountProfiles();
    void saveAccountProfiles(const QString& path);
    void displayGameAccounts(const QString& gameforgeAccount);
    void displayProfiles(const QString& gameforgeAccount);
    void addGameforgeAccount(const QString& email, const QString& password);
    void loadIdentity(const QString& path);

    Ui::MainWindow *ui;
    SettingsDialog* settingsDialog;
    GflessClient* gflessClient;
    QSystemTrayIcon* trayIcon;
    QLocalServer* gflessServer;
    QMap<QString /* gameforge account name */, Account*> accounts;
    std::shared_ptr<Identity> identity;
};

#endif // MAINWINDOW_H
