#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "NostaleAuth.h"
#include "GflessClient.h"
#include "AddAccountDialog.h"
#include "SettingsDialog.h"

#include <QMainWindow>
#include <QTimer>
#include <QSettings>
#include <QFileDialog>
#include <QVariant>
#include <QMessageBox>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool checkGameClientPath();

    void loadSettings();

    void saveSettings();

    void displayGameAccounts(const QString& gameforgeAccount);

    void addGameforgeAccount(const QString& email, const QString& password);

protected:
    void closeEvent(QCloseEvent* event);

private slots:
    void on_openAccountsButton_clicked();

    void on_addGameforgeAccountButton_clicked();

    void on_gameforgeAccountComboBox_currentTextChanged(const QString &arg1);

    void on_removeGameforgeAccountButton_clicked();

    void on_gameSettingsButton_clicked();

    void on_actionSettings_triggered();

private:
    Ui::MainWindow *ui;
    SettingsDialog* settingsDialog;

    GflessClient* gflessClient;
    //QMap<QString /* displayName */, QString /* id */> accounts;
    QMap<QString /* gameforge account name */, QMap<QString /* display name */, QString /* id */>> accounts;
    QMap<QString /* gameforge account name */, NostaleAuth*> gameforgeAccounts;
};
#endif // MAINWINDOW_H
