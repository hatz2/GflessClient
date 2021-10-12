#include "MainWindow.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    settingsDialog = new SettingsDialog(this);
    gflessClient = new GflessClient(this);
    createTrayIcon();
    loadSettings();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_openAccountsButton_clicked()
{
    if (ui->accountsListWidget->currentRow() < 0) return;
    if (!checkGameClientPath()) return;

    int openInterval = settingsDialog->getOpenInterval();
    QString gameforgeAccountUsername = ui->gameforgeAccountComboBox->currentText();
    QList<QListWidgetItem*> selectedItems = ui->accountsListWidget->selectedItems();
    NostaleAuth* nostaleAuth = gameforgeAccounts.value(gameforgeAccountUsername);
    QStringList displayNames;

    for (const auto& item : selectedItems)
        displayNames << item->text();

    for (int i = 0; i < displayNames.size(); i++)
    {
        QTimer::singleShot(openInterval * 1000 * i, [=]
        {
            QString displayName = displayNames[i];
            QString token = nostaleAuth->getToken(accounts.value(gameforgeAccountUsername).value(displayName));

            if (token.isEmpty())
                qDebug() << "Error, couldn't get token";
            else
                gflessClient->openClient(displayName, token, settingsDialog->getGameClientPath(), settingsDialog->getGameLanguage());
        });
    }

    QTimer::singleShot(displayNames.size() * openInterval * 1000, [=]
    {
        ui->statusbar->showMessage("All accounts are opened.", 10000);
    });

    ui->statusbar->showMessage("Opening selected accounts...");
}

void MainWindow::loadSettings()
{
    QSettings settings;

    settings.beginGroup("MainWindow");
    restoreGeometry(settings.value("geometry").toByteArray());
    settingsDialog->setGameClientPath(settings.value("nostale path").toString());
    settingsDialog->setOpenInterval(settings.value("open interval", 12).toInt());
    settingsDialog->setGameLanguage(settings.value("game language", 0).toInt());
    settings.endGroup();

    settings.beginGroup("Gameforge Accounts");
    QMap<QString, QVariant> accountInformation;
    accountInformation = settings.value("accounts").toMap();

    for (const auto& email : accountInformation.keys())
    {
        const QString password = accountInformation.value(email).toString();
        addGameforgeAccount(email, password);
    }
    settings.endGroup();
}

void MainWindow::saveSettings()
{
    QSettings settings;

    settings.beginGroup("MainWindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("nostale path", settingsDialog->getGameClientPath());
    settings.setValue("open interval", settingsDialog->getOpenInterval());
    settings.setValue("game language", settingsDialog->getGameLanguage());
    settings.endGroup();

    settings.beginGroup("Gameforge Accounts");
    QMap<QString, QVariant> accountInformation;

    for(int i = 0; i < ui->gameforgeAccountComboBox->count(); i++)
    {
        QString email = ui->gameforgeAccountComboBox->itemText(i);
        QString password = ui->gameforgeAccountComboBox->itemData(i).toString();

        accountInformation.insert(email, password);
    }

    settings.setValue("accounts", accountInformation);
    settings.endGroup();
}

void MainWindow::displayGameAccounts(const QString &gameforgeAccount)
{
    auto accs = accounts.value(gameforgeAccount);

    ui->accountsListWidget->clear();

    for (const auto& acc : accs.keys())
        ui->accountsListWidget->addItem(acc);
}

void MainWindow::addGameforgeAccount(const QString &email, const QString &password)
{
    NostaleAuth* nostaleAuth = new NostaleAuth(this);

    if (!nostaleAuth->authenthicate(email, password))
    {
        QMessageBox::critical(this, "Error", "Authentication failed");
        return;
    }

    QMap<QString, QString> accs = nostaleAuth->getAccounts();

    accounts.insert(email, accs);
    gameforgeAccounts.insert(email, nostaleAuth);

    ui->gameforgeAccountComboBox->addItem(email, password);
}

void MainWindow::createTrayIcon()
{
    QAction* showAction = new QAction("Show", this);
    QAction* exitAction = new QAction("Exit", this);
    QMenu* trayMenu = new QMenu(this);

    trayMenu->addAction(showAction);
    trayMenu->addAction(exitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(windowIcon());
    trayIcon->setContextMenu(trayMenu);
    trayIcon->setToolTip("Gfless Client");
    trayIcon->show();

    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::iconActivated);
    connect(showAction, &QAction::triggered, this, &MainWindow::show);
    connect(exitAction, &QAction::triggered, this, [&]()
    {
        saveSettings();
        QApplication::exit();
    });
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}


void MainWindow::on_addGameforgeAccountButton_clicked()
{
    AddAccountDialog addAccountDialog(this);
    int res = addAccountDialog.exec();

    if (res != QDialog::Accepted)
        return;

    QString email = addAccountDialog.getEmail();
    QString password = addAccountDialog.getPassword();

    addGameforgeAccount(email, password);
}


void MainWindow::on_gameforgeAccountComboBox_currentTextChanged(const QString &arg1)
{
    displayGameAccounts(arg1);
}


void MainWindow::on_removeGameforgeAccountButton_clicked()
{
    int res = QMessageBox::warning(this, "Warning", "Do you want to remove this account?\n(This will not delete your real account)", QMessageBox::Yes | QMessageBox::No);

    if (res == QMessageBox::No)
        return;

    int index = ui->gameforgeAccountComboBox->currentIndex();

    if (index < 0)
        return;

    ui->gameforgeAccountComboBox->removeItem(index);
}


void MainWindow::on_gameSettingsButton_clicked()
{
    if (!checkGameClientPath()) return;
    gflessClient->openClientSettings(settingsDialog->getGameClientPath());
}

bool MainWindow::checkGameClientPath()
{
    if (!QFile::exists(settingsDialog->getGameClientPath()))
    {
        QMessageBox::critical(this, "Error", "NostaleClientX.exe path doesn't exist");
        return false;
    }

    return true;
}


void MainWindow::on_actionSettings_triggered()
{
    settingsDialog->exec();
}


void MainWindow::on_actionAbout_3_triggered()
{

}


void MainWindow::on_actionAbout_Qt_triggered()
{
    QApplication::aboutQt();
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick)
        show();
}

