#include "mainwindow.h"
#include "ui_mainwindow.h"

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
    identity = std::make_shared<Identity>();

    ui->accountsListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->accountsListWidget, &QListWidget::customContextMenuRequested, this, &MainWindow::showContextMenu);
    connect(gflessServer, &QLocalServer::newConnection, this, &MainWindow::handleLocalConnection);
    connect(settingsDialog, &SettingsDialog::autoLoginStateChanged, ui->channelComboBox, &QComboBox::setEnabled);
    connect(settingsDialog, &SettingsDialog::autoLoginStateChanged, ui->channelLabel, &QLabel::setEnabled);
    connect(settingsDialog, &SettingsDialog::profilesPathSelected, this, &MainWindow::loadAccountProfiles);
    connect(settingsDialog, &SettingsDialog::identityPathSelected, this, &MainWindow::loadIdentity);

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
    if (!checkIdentityPath()) return;

    int openInterval = settingsDialog->getOpenInterval();
    QString gameforgeAccountUsername = ui->gameforgeAccountComboBox->currentText();
    QList<QListWidgetItem*> selectedItems = ui->accountsListWidget->selectedItems();
    Account* account = accounts.value(gameforgeAccountUsername);
    NostaleAuth* auth = account->getAuthenticator();
    QStringList displayNames;
    int profileIndex = ui->profileComboBox->currentIndex() - 1;

    for (const auto& item : qAsConst(selectedItems))
        displayNames << item->text();

    for (int i = 0; i < displayNames.size(); i++)
    {
        QTimer::singleShot(openInterval * 1000 * i, this, [=]
        {
            int gameLang = settingsDialog->getGameLanguage();
            bool autoLogin = settingsDialog->autoLogIn();
            QString gamePath = settingsDialog->getGameClientPath();
            QString displayName;
            QString id;
            QString token;

            // A profile is selected
            if (profileIndex >= 0)
            {
                displayName = account->getProfiles().at(profileIndex)->getRealName(displayNames[i]);
                id = account->getProfiles().at(profileIndex)->getId(displayNames[i]);
            }

            // No profile is selected
            else
            {
                displayName = displayNames[i];
                id = account->getAccounts().value(displayName);
            }

            token = auth->getToken(id);

            if (token.isEmpty())
            {
                qDebug() << "Error: Couldn't get token";
                ui->statusbar->showMessage("Couldn't get token");
            }

            else
            {
                gflessClient->openClient(displayName, token, gamePath, gameLang, autoLogin);
            }

        });
    }

    QTimer::singleShot(displayNames.size() * openInterval * 1000, this, [=]
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
    settingsDialog->setProfilesPath(settings.value("profiles path").toString());
    settingsDialog->setIdentityPath(settings.value("identity path").toString());
    settingsDialog->setOpenInterval(settings.value("open interval", 12).toInt());
    settingsDialog->setGameLanguage(settings.value("game language", 0).toInt());
    settingsDialog->setAutoLogin(settings.value("auto login", false).toBool());
    settingsDialog->setServerLanguage(settings.value("server language", 0).toInt());
    settingsDialog->setServer(settings.value("server", 0).toInt());
    ui->channelComboBox->setCurrentIndex(settings.value("channel", 0).toInt());

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

    loadAccountProfiles();
}

void MainWindow::saveSettings()
{
    QSettings settings;

    settings.beginGroup("MainWindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("nostale path", settingsDialog->getGameClientPath());
    settings.setValue("profiles path", settingsDialog->getProfilesPath());
    settings.setValue("identity path", settingsDialog->getIdentityPath());
    settings.setValue("open interval", settingsDialog->getOpenInterval());
    settings.setValue("game language", settingsDialog->getGameLanguageIndex());
    settings.setValue("auto login", settingsDialog->autoLogIn());
    settings.setValue("server language", settingsDialog->getServerLanguage());
    settings.setValue("server", settingsDialog->getServer());
    settings.setValue("channel", ui->channelComboBox->currentIndex());
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

void MainWindow::loadAccountProfiles()
{
    if (settingsDialog->getProfilesPath().isEmpty())
        return;

    QSettings settings(settingsDialog->getProfilesPath(), QSettings::IniFormat);
    int gfAccSize, profileSize, accSize;

    settings.beginGroup("profile_data");

    gfAccSize = settings.beginReadArray("gameforge_accounts");
    for (int i = 0; i < gfAccSize; ++i)
    {
        settings.setArrayIndex(i);
        QString gameforgeAccountUsername = settings.value("gf_account_name").toString();

        profileSize = settings.beginReadArray("profiles");
        for (int j = 0; j < profileSize; ++j)
        {
            settings.setArrayIndex(j);
            QString profileName = settings.value("profile_name").toString();

            Profile* profile = new Profile(profileName, this);

            if (accounts.value(gameforgeAccountUsername) == nullptr)
                continue;

            accounts.value(gameforgeAccountUsername)->addProfile(profile);

            accSize = settings.beginReadArray("profile_accounts");
            for (int h = 0; h < accSize; ++h)
            {
                settings.setArrayIndex(h);
                QString pseudonym = settings.value("pseudonym").toString();
                QString accountName = settings.value("account_name").toString();
                QString id = accounts.value(gameforgeAccountUsername)->getAccounts().value(accountName);

                profile->addAccount(pseudonym, accountName, id);
            }
            settings.endArray();
        }
        settings.endArray();
    }
    settings.endArray();

    settings.endGroup();

    if (ui->gameforgeAccountComboBox->currentIndex() < 0)
        return;

    displayProfiles(ui->gameforgeAccountComboBox->currentText());
}

void MainWindow::saveAccountProfiles(const QString &path)
{
    QSettings settings(path, QSettings::IniFormat);
    QList<Account*> accs = accounts.values();
    settings.beginGroup("profile_data");

    settings.beginWriteArray("gameforge_accounts");
    for (int i = 0; i < accs.size(); ++i)
    {
        settings.setArrayIndex(i);
        Account* account = accs[i];
        const QList<Profile*> profiles = account->getProfiles();

        settings.setValue("gf_account_name", account->getGameforgeAccountUsername());

        settings.beginWriteArray("profiles");
        for (int j = 0; j < profiles.size(); ++j)
        {
            settings.setArrayIndex(j);
            Profile* profile = profiles[j];
            QList<QString> profileAccounts = profile->getAccounts().keys();

            settings.setValue("profile_name", profile->getProfileName());

            settings.beginWriteArray("profile_accounts");
            for (int h = 0; h < profileAccounts.size(); ++h)
            {
                settings.setArrayIndex(h);
                QString profileAcc = profileAccounts[h];

                settings.setValue("pseudonym", profileAcc);
                settings.setValue("account_name", profile->getRealName(profileAcc));
            }
            settings.endArray();
        }
        settings.endArray();
    }
    settings.endArray();

    settings.endGroup();
}

void MainWindow::displayGameAccounts(const QString &gameforgeAccount)
{
    ui->accountsListWidget->clear();
    int profileIndex = ui->profileComboBox->currentIndex() - 1;

    if (accounts.size() <= 0)
        return;

    // Show raw accounts
    if (profileIndex < 0)
    {
        for (const auto& acc : accounts.value(gameforgeAccount)->getAccounts().keys())
            ui->accountsListWidget->addItem(acc);
    }

    // Show accounts from profile
    else
    {
        const QList<Profile*> profiles = accounts.value(gameforgeAccount)->getProfiles();
        const Profile* profile = profiles.at(profileIndex);

        for (const auto& acc : profile->getAccounts().keys())
            ui->accountsListWidget->addItem(acc);
    }
}

void MainWindow::displayProfiles(const QString &gameforgeAccount)
{
    ui->profileComboBox->clear();
    ui->profileComboBox->addItem("No profile");

    if (accounts.size() <= 0)
        return;

    for (const auto& profile : accounts.value(gameforgeAccount)->getProfiles())
        ui->profileComboBox->addItem(profile->getProfileName());
}

void MainWindow::addGameforgeAccount(const QString &email, const QString &password)
{
    NostaleAuth* nostaleAuth = new NostaleAuth(identity, this);

    if (!nostaleAuth->authenthicate(email, password))
    {
        QMessageBox::critical(this, "Error", "Authentication failed");
        return;
    }

    QMap<QString, QString> accs = nostaleAuth->getAccounts();

    Account* account = new Account(email, this);

    account->setAuthenticator(nostaleAuth);

    for (const auto& displayName : accs.keys())
        account->addRawAccount(displayName, accs.value(displayName));

    accounts.insert(email, account);

    ui->gameforgeAccountComboBox->addItem(email, password);
}

void MainWindow::loadIdentity(const QString &path)
{
    identity->load(path);
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
    connect(exitAction, &QAction::triggered, this, [=]
    {
        saveSettings();
        QApplication::exit();
    });
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    hide();
    saveSettings();
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

    QString gameforgeAccountUsername = ui->gameforgeAccountComboBox->currentText();

    accounts.remove(gameforgeAccountUsername);
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

bool MainWindow::checkIdentityPath()
{
    if (!QFile::exists(settingsDialog->getIdentityPath()))
    {
        QMessageBox::critical(this, "Error", "Identity file path doesn't exist");
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
            output = QString::number(ui->channelComboBox->currentIndex()).toLocal8Bit();

        sock->write(output);
    });
}



void MainWindow::on_addProfileButton_clicked()
{
    if (ui->gameforgeAccountComboBox->currentIndex() < 0)
        return;

    QString gameforgeAccountName = ui->gameforgeAccountComboBox->currentText();
    AddProfileDialog addProfileDialog(this);
    int res = addProfileDialog.exec();

    if (res == QDialog::Accepted)
    {
        QString profileName = addProfileDialog.getProfileName();

        Profile* profile = new Profile(profileName, this);
        accounts.value(gameforgeAccountName)->addProfile(profile);
        ui->profileComboBox->addItem(profileName);
    }
}

void MainWindow::on_profileComboBox_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    displayGameAccounts(ui->gameforgeAccountComboBox->currentText());
}

void MainWindow::showContextMenu(const QPoint& pos)
{
    if (ui->profileComboBox->currentIndex() <= 0)
        return;

    QPoint globalPos = ui->accountsListWidget->mapToGlobal(pos);

    QMenu menu(this);
    menu.addAction("Add account", this, [=]
    {
        int profileIndex = ui->profileComboBox->currentIndex() - 1;
        QStringList accountList;
        QString gameforgeAccountName = ui->gameforgeAccountComboBox->currentText();

        if (profileIndex < 0)
            return;

        for (const auto& acc : accounts.value(gameforgeAccountName)->getAccounts().keys())
            accountList << acc;

        AddProfileAccountDialog addProfileAccountDialog(accountList, this);
        int res = addProfileAccountDialog.exec();

        if (res == QDialog::Accepted)
        {
            QString gameAccount = addProfileAccountDialog.getAccountName();
            QString pseudonym = addProfileAccountDialog.getPseudonym();
            QString id = accounts.value(gameforgeAccountName)->getAccounts().value(gameAccount);

            accounts.value(gameforgeAccountName)->getProfiles().at(profileIndex)->addAccount(pseudonym, gameAccount, id);
            displayGameAccounts(gameforgeAccountName);
        }
    });

    menu.addAction("Remove selected accounts", this, [=]
    {
        int profileIndex = ui->profileComboBox->currentIndex() - 1;
        QList<QListWidgetItem*> selectedItems = ui->accountsListWidget->selectedItems();
        QString gameforgeAccountName = ui->gameforgeAccountComboBox->currentText();

        if (profileIndex < 0)
            return;

        for (const auto& item : selectedItems)
        {
            accounts.value(gameforgeAccountName)->getProfiles().at(profileIndex)->removeAccount(item->text());
        }

        displayGameAccounts(gameforgeAccountName);
    });

    menu.exec(globalPos);
}


void MainWindow::on_removeProfileButton_clicked()
{
    if (ui->gameforgeAccountComboBox->currentIndex() < 0)
        return;

    int index = ui->profileComboBox->currentIndex() - 1;

    if (index < 0)
        return;

    int res = QMessageBox::warning(this, "Remove profile", "Are you sure that you want to remove this profile?", QMessageBox::Yes | QMessageBox::No);

    if (res == QMessageBox::Yes)
    {
        QString gameforgeAccountUsername = ui->gameforgeAccountComboBox->currentText();

        accounts.value(gameforgeAccountUsername)->removeProfile(index);
        ui->profileComboBox->removeItem(index + 1);
    }
}


void MainWindow::on_actionSave_profiles_triggered()
{
    QString path = QFileDialog::getSaveFileName(this, "Save profiles", QDir::rootPath(), "(*.ini)");

    if (path.isEmpty())
        return;

    saveAccountProfiles(path);
}


void MainWindow::on_actionIdentity_generator_triggered()
{
    IdentityDialog dialog(this);
    dialog.exec();
}

