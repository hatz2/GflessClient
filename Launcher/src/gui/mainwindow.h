#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "gflessclient.h"
#include "settingsdialog.h"
#include "gameforgeaccount.h"
#include "gameaccount.h"
#include "profile.h"

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
#include <QVector>


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
    void on_addGameforgeAccountButton_clicked();

    void on_removeGameforgeAccountButton_clicked();

    void on_gameSettingsButton_clicked();

    void on_actionSettings_triggered();

    void on_actionAbout_3_triggered();

    void on_actionAbout_Qt_triggered();

    void iconActivated(QSystemTrayIcon::ActivationReason reason);

    void on_actionGet_help_triggered();

    void handleLocalConnection();

    void showContextMenu(const QPoint &pos);

    void on_actionSave_profiles_triggered();

    void on_actionIdentity_generator_triggered();

    void on_addProfileButton_clicked();

    void on_removeProfileButton_clicked();

    void on_profileComboBox_currentIndexChanged(int index);

    void on_openAccountsButton_clicked();

private:
    void createTrayIcon();
    bool checkGameClientPath();
    bool checkIdentityPath();
    void loadSettings();
    void saveSettings();
    void setupDefaultProfile();
    void loadAccountProfiles(const QString& path);
    void saveAccountProfiles(const QString& path);
    void addGameforgeAccount(const QString& email, const QString& password, const QString &identityPath);
    void displayAllAccounts();
    void displayProfile(int index);

    Ui::MainWindow *ui;
    SettingsDialog* settingsDialog;
    GflessClient* gflessClient;
    QSystemTrayIcon* trayIcon;
    QLocalServer* gflessServer;

    QVector<Profile*> profiles;
    QVector<GameforgeAccount*> gfAccounts;
    QMap<DWORD, GameAccount> processAccounts;
};

#endif // MAINWINDOW_H
