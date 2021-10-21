#include "MainWindow.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    settingsDialog = new SettingsDialog(this);
    gflessClient = new GflessClient(this);
    gflessServer = new QLocalServer(this);
    gflessServer->listen("GflessClient");
    createTrayIcon();
    loadSettings();

    connect(gflessServer, &QLocalServer::newConnection, this, &MainWindow::handleLocalConnection);
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
            QString displayName;
            QString id;

            if (ui->profileComboBox->currentIndex() > 0)
            {
                displayName = profiles.value(ui->profileComboBox->currentText()).getRealName(displayNames[i]);
                id = profiles.value(ui->profileComboBox->currentText()).getId(displayNames[i]);
            }

            else
            {
                displayName = displayNames[i];
                id = accounts.value(gameforgeAccountUsername).value(displayName);
            }

            QString token = nostaleAuth->getToken(accounts.value(gameforgeAccountUsername).value(displayName));

            if (token.isEmpty())
            {
                qDebug() << "Error, couldn't get token";
                ui->statusbar->showMessage("Couldn't get token");
            }
            else
                gflessClient->openClient(displayName, token, settingsDialog->getGameClientPath(), settingsDialog->getGameLanguage(), settingsDialog->autoLogIn());
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
    settingsDialog->setAutoLogin(settings.value("auto login", false).toBool());
    settingsDialog->setServerLanguage(settings.value("server language", 0).toInt());
    settingsDialog->setServer(settings.value("server", 0).toInt());
    settingsDialog->setChannel(settings.value("channel", 0).toInt());

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
    settings.setValue("auto login", settingsDialog->autoLogIn());
    settings.setValue("server language", settingsDialog->getServerLanguage());
    settings.setValue("server", settingsDialog->getServer());
    settings.setValue("channel", settingsDialog->getChannel());
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

void MainWindow::displayGameAccounts(const QString &gameforgeAccount, const QString& profileName)
{
    ui->accountsListWidget->clear();

    // Show the defaul accounts
    if (ui->profileComboBox->currentIndex() <= 0)
    {
        auto accs = accounts.value(gameforgeAccount).keys();

        for (const auto& acc : accs)
            ui->accountsListWidget->addItem(acc);
    }

    // Show the profile accounts
    else
    {
        if (profileName.isEmpty())
            return;

        auto profile = profiles.value(profileName);

        for (const auto& acc : profile.getAccounts().keys())
            ui->accountsListWidget->addItem(acc);
    }

}

void MainWindow::displayProfiles(const QString &gameforgeAccount)
{
    ui->profileComboBox->clear();
    ui->profileComboBox->addItem("--- Not selected ---");

    for (const auto& profile : profiles)
    {
        if (profile.getGameforgeAccount() == gameforgeAccount)
        {
            ui->profileComboBox->addItem(profile.getProfileName());
        }
    }
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
    displayProfiles(arg1);
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
    QString text = "<b>About Gfless Client</b><br><br>"
                   "Software developed by Hatz<br><br>"
                   "This application is an open source project that simulates almost everything that the Gameforge client does "
                   "allowing you to have multiple gameforge accounts in the same launcher and "
                   "open several game clients with just one click.<br><br>"
                   "You can find the source code here: "
                   "<a href=https://github.com/hatz02/GflessClient>Github</a><br><br>"
                   "Full credits to morsisko for such an amazing job reverse engineering the Gameforge Client and the login protocol.<br><br>"
                   "Sources used to make this project:<br>"
                   "<a href=https://github.com/morsisko/NosTale-Auth>Nostale-Auth</a><br>"
                   "<a href=https://github.com/morsisko/NosTale-Gfless>Nostale-Gfless</a>";

    QMessageBox::about(this, "About Gfless Client", text);
}


void MainWindow::on_actionAbout_Qt_triggered()
{
    QMessageBox::aboutQt(this);
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick)
        show();
}


void MainWindow::on_actionGet_help_triggered()
{
    QString text = "<b>Get help</b><br><br>"
                   "If you have any issue using this application you can contact me here:<br><br>"
                   "Elitepvpers: "
                   "<a href=https://www.elitepvpers.com/forum/nostale-hacks-bots-cheats-exploits/4956891-open-source-gfless-client.html>Elitepvpers thread</a><br>"
                   "Discord: Hatz#0502<br>";

    QMessageBox::about(this, "Get help", text);
}

void MainWindow::handleLocalConnection()
{
    QLocalSocket* sock = gflessServer->nextPendingConnection();
    connect(sock, &QLocalSocket::readyRead, this, [=]
    {
        QByteArray message = sock->readAll();
        QByteArray output;

        if (message == "ServerLanguage")
            output = QString::number(settingsDialog->getServerLanguage()).toLocal8Bit();

        else if (message == "Server")
            output = QString::number(settingsDialog->getServer()).toLocal8Bit();

        else if (message == "Channel")
            output = QString::number(settingsDialog->getChannel()).toLocal8Bit();

        sock->write(output);
    });
}



void MainWindow::on_addProfileButton_clicked()
{
    QString gameforgeAccountName = ui->gameforgeAccountComboBox->currentText();

    if (gameforgeAccountName.isEmpty())
        return;

    AddProfileDialog addProfileDialog(this);
    int res = addProfileDialog.exec();

    if (res == QDialog::Accepted)
    {
        QString profileName = addProfileDialog.getProfileName();

        AccountProfile profile(profileName, gameforgeAccountName);

        profiles.insert(profileName, profile);
        ui->profileComboBox->addItem(profileName);
    }
}


void MainWindow::on_profileComboBox_currentIndexChanged(const QString &arg1)
{
    displayGameAccounts(ui->gameforgeAccountComboBox->currentText(), arg1);
}


void MainWindow::on_addProfileAccountButton_clicked()
{
    if (ui->profileComboBox->currentIndex() <= 0)
        return;

    QStringList accountList;
    QString gameforgeAccountName = ui->gameforgeAccountComboBox->currentText();
    QString profileName = ui->profileComboBox->currentText();

    for (const auto& acc : accounts.value(gameforgeAccountName).keys())
        accountList << acc;

    AddProfileAccountDialog addProfileAccountDialog(accountList, this);
    int res = addProfileAccountDialog.exec();

    if (res == QDialog::Accepted)
    {
        QString gameAccount = addProfileAccountDialog.getAccountName();
        QString pseudonym = addProfileAccountDialog.getPseudonym();

        if (profiles.contains(profileName))
        {
            profiles[profileName].addAccount(pseudonym, gameAccount, accounts.value(gameforgeAccountName).value(gameAccount));
            displayGameAccounts(gameforgeAccountName, profileName);
        }
    }
}

