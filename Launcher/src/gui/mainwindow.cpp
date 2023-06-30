#include "mainwindow.h"
#include "addaccountdialog.h"
#include "addprofileaccountdialog.h"
#include "addprofiledialog.h"
#include "captchadialog.h"
#include "identitydialog.h"
#include "editmultipleprofileaccountsdialog.h"
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
    connect(settingsDialog, &SettingsDialog::profilesPathSelected, this, &MainWindow::loadAccountProfiles);

    setupDefaultProfile();
    loadSettings();
    loadAccountProfiles(settingsDialog->getProfilesPath());
}

MainWindow::~MainWindow()
{
    saveSettings();
    delete ui;
}

void MainWindow::loadSettings()
{
    QSettings settings;

    settings.beginGroup("MainWindow");
    restoreGeometry(settings.value("geometry").toByteArray());
    settingsDialog->setGameClientPath(settings.value("nostale path").toString());
    settingsDialog->setProfilesPath(settings.value("profiles path").toString());
    settingsDialog->setOpenInterval(settings.value("open interval", 12).toInt());
    settingsDialog->setGameLanguage(settings.value("game language", 0).toInt());
    settingsDialog->setTheme(settings.value("theme", 0).toInt());
    settingsDialog->setThemeComboBox(settings.value("theme", 1).toInt());
    settings.endGroup();

    settings.beginGroup("Gameforge Accounts");
    int numAccs = settings.beginReadArray("GF accounts data");

    for (int i = 0; i < numAccs; ++i) {
        settings.setArrayIndex(i);

        QString email = settings.value("email", "").toString();
        QString password = settings.value("password", "").toString();
        QString identity = settings.value("identity_path", "").toString();

        addGameforgeAccount(email, password, identity);
    }

    settings.endArray();
    settings.endGroup();

    displayAllAccounts();
}

void MainWindow::saveSettings()
{
    QSettings settings;

    settings.beginGroup("MainWindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("nostale path", settingsDialog->getGameClientPath());
    settings.setValue("profiles path", settingsDialog->getProfilesPath());
    settings.setValue("open interval", settingsDialog->getOpenInterval());
    settings.setValue("game language", settingsDialog->getGameLanguageIndex());
    settings.setValue("theme", settingsDialog->getTheme());
    settings.endGroup();

    settings.beginGroup("Gameforge Accounts");
    settings.beginWriteArray("GF accounts data");
    for (int i = 0; i < gfAccounts.size(); ++i) {
        settings.setArrayIndex(i);
        GameforgeAccount* acc = gfAccounts.at(i);

        settings.setValue("email", acc->getEmail());
        settings.setValue("password", acc->getPassword());
        settings.setValue("identity_path", acc->getIdentityPath());
    }
    settings.endArray();

    settings.endGroup();
}

void MainWindow::setupDefaultProfile()
{
    Profile* profile = new Profile("Default", this);
    profiles.push_back(profile);
    ui->profileComboBox->addItem(profile->getProfileName());
}

void MainWindow::loadAccountProfiles(const QString &path)
{
    QSettings settings(path, QSettings::IniFormat);
    int numProfiles = 0;
    int numAccs = 0;

    settings.beginGroup("profiles_data");

    numProfiles = settings.beginReadArray("profiles");

    for (int i = 0; i < numProfiles; ++i) {
        settings.setArrayIndex(i);

        QString profileName = settings.value("profile_name", "").toString();

        Profile* profile = new Profile(profileName, this);
        profiles.push_back(profile);
        ui->profileComboBox->addItem(profileName);

        numAccs = settings.beginReadArray("game_accounts");

        for (int j = 0; j < numAccs; ++j) {
            settings.setArrayIndex(j);

            QString email = settings.value("email", "").toString();
            QString accountName = settings.value("account_name", "").toString();
            QString id = settings.value("id", "").toString();
            QString displayName = settings.value("display_name", "").toString();
            int serverLocation = settings.value("server_location", "").toInt();
            int server = settings.value("server", "").toInt();
            int channel = settings.value("channel", "").toInt();
            int character = settings.value("character", "").toInt();
            bool autoLogin = settings.value("auto_login", false).toBool();

            GameforgeAccount* gfacc = nullptr;

            for (GameforgeAccount* gfaccount : gfAccounts) {
                if (gfaccount->getEmail() == email) {
                    gfacc = gfaccount;
                    break;
                }
            }

            if (gfacc == nullptr)
                continue;

            GameAccount gameAcc(gfacc, accountName, id, displayName, serverLocation, server, channel, character, autoLogin);

            profile->addAccount(gameAcc);
        }

        settings.endArray();
    }

    settings.endArray();

    settings.endGroup();
}

void MainWindow::saveAccountProfiles(const QString &path)
{
    QSettings settings(path, QSettings::IniFormat);

    settings.beginGroup("profiles_data");

    settings.beginWriteArray("profiles");
    for (int i = 1; i < profiles.size(); ++i) {
        settings.setArrayIndex(i-1);

        settings.setValue("profile_name", profiles[i]->getProfileName());

        QVector<GameAccount> accs = profiles[i]->getAccounts();

        settings.beginWriteArray("game_accounts");
        for (int j = 0; j < accs.size(); ++j) {
            settings.setArrayIndex(j);

            settings.setValue("email", accs[j].getGfAcc()->getEmail());
            settings.setValue("account_name", accs[j].getName());
            settings.setValue("id", accs[j].getId());
            settings.setValue("display_name", accs[j].getDisplayName());
            settings.setValue("server_location", accs[j].getServerLocation());
            settings.setValue("server", accs[j].getServer());
            settings.setValue("channel", accs[j].getChannel());
            settings.setValue("character", accs[j].getSlot());
            settings.setValue("auto_login", accs[j].getAutoLogin());
        }
        settings.endArray();
    }
    settings.endArray();

    settings.endGroup();
}

void MainWindow::addGameforgeAccount(const QString &email, const QString &password, const QString& identityPath)
{
    bool captcha = false;
    bool wrongCredentials = false;
    QString gfChallengeId;
    GameforgeAccount* gfAcc = new GameforgeAccount(email, password, identityPath, this);

    if (!gfAcc->authenticate(captcha, gfChallengeId, wrongCredentials)) {
        if (captcha) {
            CaptchaDialog captcha(gfChallengeId, this);
            int res = captcha.exec();

            if (res == QDialog::Accepted) {
                addGameforgeAccount(email, password, identityPath);
            }
        }
        else if (wrongCredentials) {
            QMessageBox::critical(this, "Error", "Incorrect username or password");
        }
        else {
            QMessageBox::critical(this, "Error", "Authentication failed");
        }

        delete gfAcc;
        return;
    }

    gfAccounts.push_back(gfAcc);

    ui->gameforgeAccountComboBox->addItem(email);


    // Update default profile

    QMap<QString, QString> gameAccs = gfAcc->getGameAccounts();

    for (auto it = gameAccs.begin(); it != gameAccs.end(); ++it) {
        GameAccount gameAccount(gfAcc, it.value(), it.key());
        profiles.first()->addAccount(gameAccount);
    }

    displayProfile(ui->profileComboBox->currentIndex());
}

void MainWindow::displayAllAccounts()
{
    ui->accountsListWidget->clear();

    for (const GameforgeAccount* gfAcc : gfAccounts) {
        QMap<QString, QString> gameAccs = gfAcc->getGameAccounts();

        for (auto it = gameAccs.begin(); it != gameAccs.end(); ++it) {
            QListWidgetItem* item = new QListWidgetItem(ui->accountsListWidget);
            item->setText(it.value());
            item->setData(Qt::UserRole, it.key());
        }
    }
}

void MainWindow::displayProfile(int index)
{
    if (index < 0)
        return;

    Profile* profile = profiles[index];

    auto accounts = profile->getAccounts();


    ui->accountsListWidget->clear();
    for (const GameAccount& acc : accounts) {
        QListWidgetItem* item = new QListWidgetItem(ui->accountsListWidget);
        item->setText(acc.getDisplayName());
        item->setData(Qt::UserRole, acc.getId());
    }
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
    QString identityPath = addAccountDialog.getIdentityPath();

    addGameforgeAccount(email, password, identityPath);
}


void MainWindow::on_removeGameforgeAccountButton_clicked()
{
    int res = QMessageBox::warning(this, "Warning", "Do you want to remove this account?\n(This will not delete your real account)", QMessageBox::Yes | QMessageBox::No);

    if (res == QMessageBox::No)
        return;

    int index = ui->gameforgeAccountComboBox->currentIndex();

    if (index < 0)
        return;

    gfAccounts.remove(index);
    ui->gameforgeAccountComboBox->removeItem(index);

    displayProfile(ui->profileComboBox->currentIndex());
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
                   "Full credits to morsisko and stdLemon for such an amazing job reverse engineering the Gameforge Client and the login protocol.<br><br>"
                   "Sources used to make this project:<br>"
                   "<a href=https://github.com/morsisko/NosTale-Auth>Nostale-Auth</a><br>"
                   "<a href=https://github.com/morsisko/NosTale-Gfless>Nostale-Gfless</a><br>"
                   "<a href=https://github.com/stdLemon/nostale-auth>nostale-auth</a>";

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
                   "<a href=https://www.elitepvpers.com/forum/nostale-hacks-bots-cheats-exploits/4956891-open-source-gfless-client.html>Elitepvpers thread</a><br>";

    QMessageBox::about(this, "Get help", text);
}

void MainWindow::handleLocalConnection()
{
    QLocalSocket* sock = gflessServer->nextPendingConnection();
    connect(sock, &QLocalSocket::readyRead, this, [=]
    {
        QString data = sock->readAll();
        QByteArray output;

        QStringList splitted = data.split(' ');

        if (splitted.size() != 2) {
            sock->write("0");
            return;
        }

        DWORD pid = splitted[0].toUInt();
        QString message = splitted[1];

        // Get information asociated with the account of this process
        if (!processAccounts.contains(pid)) {
            sock->write("0");
            return;
        }

        const GameAccount& acc = processAccounts[pid];

        if (message == "ServerLanguage")
            output = QString::number(acc.getServerLocation()).toLocal8Bit();

        else if (message == "Server")
            output = QString::number(acc.getServer()).toLocal8Bit();

        else if (message == "Channel")
            output = QString::number(acc.getChannel()).toLocal8Bit();

        else if (message == "Character")
            output = QString::number(acc.getSlot()).toLocal8Bit();

        sock->write(output);
    });
}

void MainWindow::showContextMenu(const QPoint& pos)
{
    if (ui->profileComboBox->currentIndex() <= 0)
        return;

    QPoint globalPos = ui->accountsListWidget->mapToGlobal(pos);

    QMenu menu(this);

    menu.addAction("Add account", this, [=]
    {
        int profileIndex = ui->profileComboBox->currentIndex();

        if (profileIndex <= 0)
            return;

        AddProfileAccountDialog dialog(gfAccounts, this);

        int res = dialog.exec();

        if (res == QDialog::Accepted) {
            Profile* profile = profiles[profileIndex];

            GameAccount acc(
                dialog.getGfAcc(),
                dialog.getAccountName(),
                dialog.getId(),
                dialog.getPseudonym(),
                dialog.getServerLocation(),
                dialog.getServer(),
                dialog.getChannel(),
                dialog.getCharacter(),
                dialog.getAutoLogin()
                );

            profile->addAccount(acc);

            displayProfile(profileIndex);
        }
    });

    if (ui->accountsListWidget->selectedItems().count() > 0) {
        menu.addAction("Remove selected accounts", this, [=]
        {
            int profileIndex = ui->profileComboBox->currentIndex();
            QList<QListWidgetItem*> selectedItems = ui->accountsListWidget->selectedItems();

            if (profileIndex <= 0)
               return;

            Profile* profile = profiles[profileIndex];

            for (auto it = selectedItems.crbegin(); it != selectedItems.crend(); ++it) {
               int row = ui->accountsListWidget->row(*it);
               profile->removeAccount(row);
            }

            displayProfile(profileIndex);
        });
    }

    if (ui->accountsListWidget->selectedItems().count() == 1) {
        menu.addAction("Edit selected account", this, [=]() {
            int profileIndex = ui->profileComboBox->currentIndex();
            QList<QListWidgetItem*> selectedItems = ui->accountsListWidget->selectedItems();
            QListWidgetItem* item = selectedItems.first();

            if (profileIndex <= 0)
                return;

            Profile* profile = profiles[profileIndex];
            int row = ui->accountsListWidget->row(item);

            if (row < 0)
                return;

            const GameAccount& gameAcc = profile->getAccounts().at(row);

            AddProfileAccountDialog dialog(gfAccounts, gameAcc, this);

            int res = dialog.exec();

            if (res == QDialog::Accepted) {
                GameAccount newAccount(
                    dialog.getGfAcc(),
                    dialog.getAccountName(),
                    dialog.getId(),
                    dialog.getPseudonym(),
                    dialog.getServerLocation(),
                    dialog.getServer(),
                    dialog.getChannel(),
                    dialog.getCharacter(),
                    dialog.getAutoLogin()
                    );

                profile->editAccount(row, newAccount);
                displayProfile(profileIndex);
            }
        });

        if (ui->profileComboBox->currentIndex() != 0) {
            menu.addAction("Move up", this, [=]() {
                int profileIndex = ui->profileComboBox->currentIndex();
                Profile* profile = profiles[profileIndex];

                int row = ui->accountsListWidget->currentRow();

                if (row <= 0)
                    return;

                profile->moveAccountUp(row);
                displayProfile(profileIndex);
                ui->accountsListWidget->setCurrentRow(row - 1);
            });

            menu.addAction("Move down", this, [=]() {
                int profileIndex = ui->profileComboBox->currentIndex();
                Profile* profile = profiles[profileIndex];

                int row = ui->accountsListWidget->currentRow();

                if (row < 0 || row >= ui->accountsListWidget->count()-1)
                    return;

                profile->moveAccountDown(row);
                displayProfile(profileIndex);
                ui->accountsListWidget->setCurrentRow(row + 1);

            });
        }

    }
    else if (ui->accountsListWidget->selectedItems().count() > 1) {
        menu.addAction("Edit login information of all selected acccounts", this, [=]() {
            int profileIndex = ui->profileComboBox->currentIndex();
            QList<QListWidgetItem*> selectedItems = ui->accountsListWidget->selectedItems();

            if (profileIndex <= 0)
                return;

            Profile* profile = profiles[profileIndex];


            EditMultipleProfileAccountsDialog dialog(this);
            int res = dialog.exec();

            if (res == QDialog::Accepted) {
                int serverLocation = dialog.getServerLocation();
                int server = dialog.getServer();
                int channel = dialog.getChannel();
                int character = dialog.getCharacter();
                bool login = dialog.getAutoLogin();


                for (const auto item : selectedItems) {
                    int row = ui->accountsListWidget->row(item);

                    GameAccount gameAcc = profile->getAccounts().at(row);

                    gameAcc.setServerLocation(serverLocation);
                    gameAcc.setServer(server);
                    gameAcc.setChannel(channel);
                    gameAcc.setSlot(character);
                    gameAcc.setAutoLogin(login);

                    profile->editAccount(row, gameAcc);
                }

                displayProfile(profileIndex);
            }

        });
    }

    menu.exec(globalPos);
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


void MainWindow::on_addProfileButton_clicked()
{
    AddProfileDialog dialog(this);
    int res = dialog.exec();

    if (res == QDialog::Accepted) {
        QString profileName = dialog.getProfileName();

        // Check if a profile with this name already exists
        for (const auto& p : profiles) {
            if (p->getProfileName() == profileName) {
                QMessageBox::critical(this, "Error", "A profile with the same name already exists");
                return;
            }
        }

        Profile* profile = new Profile(profileName, this);
        profiles.push_back(profile);
        ui->profileComboBox->addItem(profileName);

        ui->profileComboBox->setCurrentIndex(ui->profileComboBox->count() - 1);
    }
}


void MainWindow::on_removeProfileButton_clicked()
{
    int index = ui->profileComboBox->currentIndex();

    if (index <= 0)
        return;

    int res = QMessageBox::warning(this, "Remove profile", "Are you sure that you want to remove this profile?", QMessageBox::Yes | QMessageBox::No);

    if (res == QMessageBox::Yes)
    {
        profiles.remove(index);
        ui->profileComboBox->removeItem(index);
    }
}


void MainWindow::on_profileComboBox_currentIndexChanged(int index)
{
    ui->accountsListWidget->clear();

    displayProfile(index);
}


void MainWindow::on_openAccountsButton_clicked()
{
    if (ui->accountsListWidget->currentRow() < 0)
        return;

    if (!checkGameClientPath())
        return;

    int openInterval = settingsDialog->getOpenInterval();
    Profile* profile = profiles[ui->profileComboBox->currentIndex()];
    GameAccount gameAccount;
    QString gamePath = settingsDialog->getGameClientPath();
    int gameLang = settingsDialog->getGameLanguage();

    for (int i = 0; i < ui->accountsListWidget->selectedItems().count(); ++i) {
        int row = ui->accountsListWidget->row(ui->accountsListWidget->selectedItems().at(i));

        QTimer::singleShot(openInterval * 1000 * i, this, [=](){
            const GameAccount& gameAccount = profile->getAccounts().at(row);

            QString token = gameAccount.getGfAcc()->getToken(gameAccount.getId());

            if (token.isEmpty()) {
                qDebug() << "Error: Couldn't get token";
                ui->statusbar->showMessage("Couldn't get token", 10000);
            }
            else {
                DWORD pid = 0;
                if (gflessClient->openClient(gameAccount.getName(), token, gamePath, gameLang, gameAccount.getAutoLogin(), pid)) {
                    QString msg = "Launched game with PID " + QString::number(pid);
                    ui->statusbar->showMessage(msg, 10000);


                    processAccounts.insert(pid, gameAccount);
                }
                else {
                    ui->statusbar->showMessage("Failed to launch game", 10000);
                }
            }
        });
    }
}

