#include "mainwindow.h"
#include "addaccountdialog.h"
#include "addprofileaccountdialog.h"
#include "addprofiledialog.h"
#include "captchadialog.h"
#include "identitydialog.h"
#include "editmultipleprofileaccountsdialog.h"
#include "editproxydialog.h"
#include "gameupdatedialog.h"
#include "creategameaccountdialog.h"
#include <QBrush>
#include <QCheckBox>
#include <QColor>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QSet>
#include <QQueue>
#include <QStandardPaths>
#include <QTextStream>
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
    setupProxyControls();

    setWindowTitle("Gfless Client v" + QString(APP_VERSION));

    ui->accountsListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->accountsListWidget, &QListWidget::customContextMenuRequested, this, &MainWindow::showContextMenu);
    connect(gflessServer, &QLocalServer::newConnection, this, &MainWindow::handleLocalConnection);
    connect(settingsDialog, &SettingsDialog::profilesPathSelected, this, &MainWindow::loadAccountProfiles);

    setupDefaultProfile();
    loadSettings();
    loadAccountProfiles(settingsDialog->getProfilesPath());

    if (settingsDialog->getCheckUpdates())
        updateGame();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadSettings()
{
    loadingStoredAccounts = true;
    QSettings settings;

    settings.beginGroup("MainWindow");
    restoreGeometry(settings.value("geometry").toByteArray());
    settingsDialog->setGameClientPath(settings.value("nostale path").toString());
    settingsDialog->setProfilesPath(settings.value("profiles path").toString());
    settingsDialog->setOpenInterval(settings.value("open interval", 10).toInt());
    settingsDialog->setGameLanguage(settings.value("game language", 0).toInt());
    settingsDialog->setTheme(settings.value("theme", 0).toInt());
    settingsDialog->setThemeComboBox(settings.value("theme", 0).toInt());
    settingsDialog->setDisabledNosmall(settings.value("disable_nosmall", false).toBool());
    settingsDialog->setCheckUpdates(settings.value("check_updates", true).toBool());
    settingsDialog->setConfirmRemoveAccount(settings.value("confirm_remove_account", true).toBool());

    defaultAutoLogin = settings.value("default_autologin", false).toBool();
    defaultServerLocation = settings.value("default_serverlocation", 0).toInt();
    defaultServer = settings.value("default_server", 0).toInt();
    defaultChannel = settings.value("default_channel", 0).toInt();
    defaultCharacter = settings.value("default_character", 0).toInt();
    useProxiesGlobally = settings.value("use_proxies_globally", true).toBool();

    settings.endGroup();

    settings.beginGroup("Gameforge Accounts");
    int numAccs = settings.beginReadArray("GF accounts data");

    for (int i = 0; i < numAccs; ++i) {
        settings.setArrayIndex(i);

        QString email = settings.value("email", "").toString();
        QString password = settings.value("password", "").toString();
        QString token = settings.value("token", "").toString();
        QString identity = settings.value("identity_path", "").toString();
        QString installationId = settings.value("installation_id", "").toString();
        QString proxyIp = settings.value("proxy_ip", "").toString();
        QString socksPort = settings.value("socks_port", "").toString();
        QString proxyUsername = settings.value("proxy_username", "").toString();
        QString proxyPassword = settings.value("proxy_password", "").toString();
        QString customClientPath = settings.value("custom_client", "").toString();
        bool useProxy = settings.value("use_proxy", false).toBool();

        // Always load persisted accounts without forcing online authentication at startup.
        // This prevents account loss when auth/proxy fails or app closes during startup.
        addGameforgeAccount(email, password, token, identity, installationId, customClientPath, proxyIp, socksPort, proxyUsername, proxyPassword, useProxy);
    }

    settings.endArray();
    settings.endGroup();

    applyGlobalProxyMode();
    updateProxyModeButtonText();
    updateAllGameforgeAccountVisuals();
    writeAccountIpsJson();
#ifdef NO_PROXY_MODE
    syncProxifierProfile();
#endif
    displayProfile(ui->profileComboBox->currentIndex());
    loadingStoredAccounts = false;
}

void MainWindow::saveSettings()
{
    if (loadingStoredAccounts) {
        return;
    }

    QSettings settings;

    settings.beginGroup("MainWindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("nostale path", settingsDialog->getGameClientPath());
    settings.setValue("profiles path", settingsDialog->getProfilesPath());
    settings.setValue("open interval", settingsDialog->getOpenInterval());
    settings.setValue("game language", settingsDialog->getGameLanguageIndex());
    settings.setValue("theme", settingsDialog->getTheme());
    settings.setValue("disable_nosmall", settingsDialog->getDisabledNosmall());
    settings.setValue("check_updates", settingsDialog->getCheckUpdates());
    settings.setValue("confirm_remove_account", settingsDialog->getConfirmRemoveAccount());
    settings.setValue("default_autologin", defaultAutoLogin);
    settings.setValue("default_serverlocation", defaultServerLocation);
    settings.setValue("default_server", defaultServer);
    settings.setValue("default_channel", defaultChannel);
    settings.setValue("default_character", defaultCharacter);
    settings.setValue("use_proxies_globally", useProxiesGlobally);
    settings.endGroup();

    settings.beginGroup("Gameforge Accounts");
    settings.beginWriteArray("GF accounts data");
    for (int i = 0; i < gfAccounts.size(); ++i) {
        settings.setArrayIndex(i);
        GameforgeAccount* acc = gfAccounts.at(i);

        settings.setValue("email", acc->getEmail());
        settings.setValue("password", acc->getPassword());
        settings.setValue("token", acc->getAuth()->getToken());
        settings.setValue("identity_path", acc->getIdentityPath());
        settings.setValue("installation_id", acc->getAuth()->getInstallationId());
        settings.setValue("custom_client", acc->getcustomClientPath());

        settings.setValue("use_proxy", acc->getAuth()->getUseProxy());
        settings.setValue("proxy_ip", acc->getAuth()->getProxyIp());
        settings.setValue("socks_port", acc->getAuth()->getSocksPort());
        settings.setValue("proxy_username", acc->getAuth()->getProxyUsername());
        settings.setValue("proxy_password", acc->getAuth()->getProxyPassword());
    }

    settings.endArray();

    settings.endGroup();
    writeAccountIpsJson();
#ifdef NO_PROXY_MODE
    syncProxifierProfile();
#endif
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

void MainWindow::addGameforgeAccount(const QString &email, const QString &password, const QString& identityPath, const QString &installationId, const QString &customClientPath, const QString &proxyIp, const QString &socksPort, const QString &proxyUsername, const QString &proxyPassword, const bool useProxy)
{
    bool captcha = false;
    bool wrongCredentials = false;
    QString gfChallengeId;
    bool authenticated = false;
    bool switchedToNoProxy = false;
    GameforgeAccount* gfAcc = new GameforgeAccount(
        email,
        password,
        identityPath,
        installationId,
        customClientPath,
        useProxy,
        proxyIp,
        socksPort,
        proxyUsername,
        proxyPassword,
        this
    );
    gfAcc->getAuth()->setForceNoProxy(!useProxiesGlobally);

    authenticated = gfAcc->authenticate(captcha, gfChallengeId, wrongCredentials);

#ifdef NO_PROXY_MODE
    // In NoIP build, only disable proxy for accounts that fail through proxy.
    if (!authenticated && useProxy && !captcha && !wrongCredentials) {
        gfAcc->setProxyConfig(false, "", "", "", "");
        gfAcc->getAuth()->setForceNoProxy(!useProxiesGlobally);

        bool retryCaptcha = false;
        bool retryWrongCredentials = false;
        QString retryChallengeId;
        const bool retryAuthenticated = gfAcc->authenticate(retryCaptcha, retryChallengeId, retryWrongCredentials);

        if (retryAuthenticated) {
            authenticated = true;
            switchedToNoProxy = true;
        } else {
            // Keep original proxy config if no-proxy retry did not solve it.
            gfAcc->setProxyConfig(useProxy, proxyIp, socksPort, proxyUsername, proxyPassword);
            gfAcc->getAuth()->setForceNoProxy(!useProxiesGlobally);
            captcha = retryCaptcha;
            wrongCredentials = retryWrongCredentials;
            gfChallengeId = retryChallengeId;
        }
    }
#endif

    if (!authenticated) {
        if (captcha) {
            CaptchaDialog captcha(gfChallengeId, gfAcc->getAuth()->getNetworkManager(), this);
            int res = captcha.exec();

            if (res == QDialog::Accepted) {
                addGameforgeAccount(email, password, identityPath, installationId, customClientPath, proxyIp, socksPort, proxyUsername, proxyPassword, useProxy);
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
    updateGameforgeAccountVisual(gfAccounts.size() - 1);

    if (switchedToNoProxy) {
        ui->statusbar->showMessage(
            "Proxy failed for " + email + ". Account loaded with no proxy.",
            12000
        );
    }


    // Update default profile
    QMap<QString, QString> gameAccs = gfAcc->getGameAccounts();

    for (auto it = gameAccs.begin(); it != gameAccs.end(); ++it) {
        GameAccount gameAccount(gfAcc, it.value(), it.key(), it.value(), defaultServerLocation, defaultServer, defaultChannel, defaultCharacter, defaultAutoLogin);
        profiles.first()->addAccount(gameAccount);
    }

    writeAccountIpsJson();
    displayProfile(ui->profileComboBox->currentIndex());
}

void MainWindow::addGameforgeAccount(const QString &email, const QString& password, const QString &token, const QString &identityPath, const QString &installationId, const QString &customClientPath, const QString &proxyIp, const QString &socksPort, const QString &proxyUsername, const QString &proxyPassword, const bool useProxy)
{
    bool switchedToNoProxy = false;
    GameforgeAccount* gfAcc = new GameforgeAccount(
        email,
        password,
        identityPath,
        installationId,
        customClientPath,
        useProxy,
        proxyIp,
        socksPort,
        proxyUsername,
        proxyPassword,
        this
    );
    gfAcc->getAuth()->setForceNoProxy(!useProxiesGlobally);

    gfAcc->setToken(token);

    // Update default profile
    gfAcc->updateGameAccounts();
    QMap<QString, QString> gameAccs = gfAcc->getGameAccounts();

#ifdef NO_PROXY_MODE
    // In NoIP build, fallback to no proxy only when proxy path fails to load accounts.
    if (useProxy && gameAccs.isEmpty()) {
        gfAcc->setProxyConfig(false, "", "", "", "");
        gfAcc->getAuth()->setForceNoProxy(!useProxiesGlobally);
        gfAcc->updateGameAccounts();
        QMap<QString, QString> fallbackAccounts = gfAcc->getGameAccounts();

        if (!fallbackAccounts.isEmpty()) {
            gameAccs = fallbackAccounts;
            switchedToNoProxy = true;
        } else {
            // Keep original proxy config if no-proxy retry did not improve loading.
            gfAcc->setProxyConfig(useProxy, proxyIp, socksPort, proxyUsername, proxyPassword);
            gfAcc->getAuth()->setForceNoProxy(!useProxiesGlobally);
            gfAcc->updateGameAccounts();
            gameAccs = gfAcc->getGameAccounts();
        }
    }
#endif

    gfAccounts.push_back(gfAcc);

    ui->gameforgeAccountComboBox->addItem(email);
    updateGameforgeAccountVisual(gfAccounts.size() - 1);

    if (switchedToNoProxy) {
        ui->statusbar->showMessage(
            "Proxy failed for " + email + ". Account loaded with no proxy.",
            12000
        );
    }

    for (auto it = gameAccs.begin(); it != gameAccs.end(); ++it) {
        GameAccount gameAccount(gfAcc, it.value(), it.key(), it.value(), defaultServerLocation, defaultServer, defaultChannel, defaultCharacter, defaultAutoLogin);
        profiles.first()->addAccount(gameAccount);
    }

    writeAccountIpsJson();
    displayProfile(ui->profileComboBox->currentIndex());
}

void MainWindow::removeAccountsFromDefaultProfile(const QString &email)
{
    if (profiles.count() <= 0)
        return;

    Profile* defaultProfile = profiles[0];

    auto accs = defaultProfile->getAccounts();

    for (int i = accs.count() - 1; i >= 0; --i) {
        if (accs[i].getGfAcc()->getEmail() == email) {
            defaultProfile->removeAccount(i);
        }
    }
}

void MainWindow::displayProfile(int index)
{
    if (index < 0)
        return;

    Profile* profile = profiles[index];

    if (index == 0) {
        profile->sort();
    }

    auto accounts = profile->getAccounts();

    ui->accountsListWidget->clear();
    for (const GameAccount& acc : accounts) {
        QListWidgetItem* item = new QListWidgetItem(ui->accountsListWidget);
        item->setText(acc.toString());
        item->setData(Qt::UserRole, acc.getId());
        const bool hasProxy = acc.getGfAcc()->getAuth()->getUseProxy();
        item->setForeground(hasProxy ? QColor(0, 140, 0) : QColor(180, 30, 30));
    }
}

void MainWindow::updateGame()
{
    QString gameDir = settingsDialog->getGameClientPath();

    if (gameDir.isEmpty())
        return;

    gameDir = gameDir.left(gameDir.lastIndexOf("/"));

    GameUpdater updater(gfAccounts, gameDir, this);

    GameUpdateDialog updateDialog(&updater, this);
    updateDialog.exec();
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
    connect(exitAction, &QAction::triggered, this, [&]
    {
        saveSettings();
        QApplication::exit();
    });
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    hide();
    if (!loadingStoredAccounts) {
        saveSettings();
    }
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
    QString installationId = addAccountDialog.getInstallationId();
    QString proxyIp = addAccountDialog.getProxyIp();
    QString socksPort = addAccountDialog.getSocksPort();
    QString proxyUsername = addAccountDialog.getProxyUsername();
    QString proxyPassword = addAccountDialog.getProxyPassword();
    QString customClient = addAccountDialog.getCustomClientPath();
    bool useProxy = addAccountDialog.getUseProxy();

    addGameforgeAccount(email, password, identityPath, installationId, customClient, proxyIp, socksPort, proxyUsername, proxyPassword, useProxy);
}


void MainWindow::on_removeGameforgeAccountButton_clicked()
{
    int index = ui->gameforgeAccountComboBox->currentIndex();

    if (index < 0)
        return;

    const QString email = ui->gameforgeAccountComboBox->currentText();

    if (settingsDialog->getConfirmRemoveAccount()) {
        QMessageBox box(this);
        box.setIcon(QMessageBox::Warning);
        box.setWindowTitle("Warning");
        box.setText("Do you want to remove the account " + email + "?\n(This will not delete your real account)");
        box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        box.setDefaultButton(QMessageBox::No);

        QCheckBox* dontAskCheckBox = new QCheckBox("Don't ask me again", &box);
        box.setCheckBox(dontAskCheckBox);

        const int res = box.exec();

        if (res != QMessageBox::Yes)
            return;

        if (dontAskCheckBox->isChecked()) {
            settingsDialog->setConfirmRemoveAccount(false);
        }
    }

    removeAccountsFromDefaultProfile(email);

    gfAccounts.remove(index);
    ui->gameforgeAccountComboBox->removeItem(index);

    writeAccountIpsJson();
    saveSettings();
    displayProfile(ui->profileComboBox->currentIndex());
}

void MainWindow::on_editGameforgeAccountButton_clicked()
{
    int index = ui->gameforgeAccountComboBox->currentIndex();
    if (index < 0 || index >= gfAccounts.size()) {
        return;
    }

    GameforgeAccount* acc = gfAccounts[index];
    NostaleAuth* auth = acc->getAuth();

    EditProxyDialog dialog(this);
    dialog.setValues(
        auth->getUseProxy(),
        auth->getProxyIp(),
        auth->getSocksPort(),
        auth->getProxyUsername(),
        auth->getProxyPassword(),
        acc->getIdentityPath(),
        acc->getcustomClientPath(),
        auth->getInstallationId()
    );

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    acc->setProxyConfig(
        dialog.getUseProxy(),
        dialog.getProxyIp().trimmed(),
        dialog.getSocksPort().trimmed(),
        dialog.getProxyUsername().trimmed(),
        dialog.getProxyPassword()
    );
    acc->setAdvancedConfig(
        dialog.getIdentityPath(),
        dialog.getInstallationId(),
        dialog.getCustomGamePath()
    );

    // Keep global mode behavior after editing.
    acc->getAuth()->setForceNoProxy(!useProxiesGlobally);

    saveSettings();
    updateGameforgeAccountVisual(index);
    if (dialog.getUseProxy()) {
        ui->statusbar->showMessage("Proxy settings updated for " + acc->getEmail(), 8000);
    }
    on_gameforgeAccountComboBox_currentIndexChanged(index);
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
                   "<a href=https://www.elitepvpers.com/forum/nostale-hacks-bots-cheats-exploits/4956891-open-source-gfless-client.html>Elitepvpers thread</a><br>"
                   "Discord: "
                   "<a href=https://discord.gg/AVs6g3myx3>Gfless</a><br>";

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

        if (message == "AutoLogin")
            output = QString::number(acc.getAutoLogin()).toLocal8Bit();

        else if (message == "ServerLanguage")
            output = QString::number(acc.getServerLocation()).toLocal8Bit();

        else if (message == "Server")
            output = QString::number(acc.getServer()).toLocal8Bit();

        else if (message == "Channel")
            output = QString::number(acc.getChannel()).toLocal8Bit();

        else if (message == "Character")
            output = QString::number(acc.getSlot()).toLocal8Bit();

        else if (message == "DisableNosmall")
            output = QString::number(settingsDialog->getDisabledNosmall()).toLocal8Bit();

        sock->write(output);
    });
}

void MainWindow::showContextMenu(const QPoint& pos)
{
    int profileIndex = ui->profileComboBox->currentIndex();

    QPoint globalPos = ui->accountsListWidget->mapToGlobal(pos);

    QMenu menu(this);

    if (profileIndex > 0) {
        menu.addAction("Add account", this, [=]
        {
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
                    Profile* profile = profiles[profileIndex];

                    int row = ui->accountsListWidget->currentRow();

                    if (row <= 0)
                        return;

                    profile->moveAccountUp(row);
                    displayProfile(profileIndex);
                    ui->accountsListWidget->setCurrentRow(row - 1);
                });

                menu.addAction("Move down", this, [=]() {
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
    }

    if (ui->accountsListWidget->selectedItems().count() > 1 || profileIndex == 0) {
        menu.addAction("Edit login information of all selected acccounts", this, [=]() {
            QList<QListWidgetItem*> selectedItems = ui->accountsListWidget->selectedItems();
            Profile* profile = profiles[profileIndex];

            // Check if the selected accounts have any field in common
            bool sharedAutoLogin = true;
            bool sharedServerLocation = true;
            bool sharedServer = true;
            bool sharedChannel = true;
            bool sharedCharacter = true;

            GameAccount firstAcc = profile->getAccounts().at(ui->accountsListWidget->row(selectedItems.first()));

            for (auto item : selectedItems) {
                GameAccount acc = profile->getAccounts().at(ui->accountsListWidget->row(item));

                if (acc.getAutoLogin() != firstAcc.getAutoLogin())
                    sharedAutoLogin = false;

                if (acc.getServerLocation() != firstAcc.getServerLocation())
                    sharedServerLocation = false;

                if (acc.getServer() != firstAcc.getServer())
                    sharedServer = false;

                if (acc.getChannel() != firstAcc.getChannel())
                    sharedChannel = false;

                if (acc.getSlot() != firstAcc.getSlot())
                    sharedCharacter = false;
            }

            EditMultipleProfileAccountsDialog dialog(
                sharedAutoLogin ? firstAcc.getAutoLogin() : false,
                sharedServerLocation ? firstAcc.getServerLocation() : 0,
                sharedServer ? firstAcc.getServer() : 0,
                sharedChannel ? firstAcc.getChannel() : 0,
                sharedCharacter ? firstAcc.getSlot() : 5,
                this
                );

            int res = dialog.exec();

            if (res == QDialog::Accepted) {
                int serverLocation = dialog.getServerLocation();
                int server = dialog.getServer();
                int channel = dialog.getChannel();
                int character = dialog.getCharacter();
                bool login = dialog.getAutoLogin();

                if (profileIndex == 0) { // Default profile
                    defaultAutoLogin = login;
                    defaultServerLocation = serverLocation;
                    defaultServer = server;
                    defaultChannel = channel;

                    if (character != 5)
                        defaultCharacter = character;
                }

                for (const auto item : selectedItems) {
                    int row = ui->accountsListWidget->row(item);

                    GameAccount gameAcc = profile->getAccounts().at(row);

                    gameAcc.setServerLocation(serverLocation);
                    gameAcc.setServer(server);
                    gameAcc.setChannel(channel);
                    gameAcc.setSlot(character == 5 ? gameAcc.getSlot() : character);
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

void MainWindow::on_actionExport_triggered()
{
    const QString path = QDir(QCoreApplication::applicationDirPath()).filePath("accounts.json");

    if (exportData(path)) {
        QMessageBox::information(this, "Export", "Exported to:\n" + QDir::toNativeSeparators(path));
    }
    else {
        QMessageBox::critical(this, "Export", "Could not write the export file.");
    }
}

void MainWindow::on_actionImport_triggered()
{
    const QString defaultPath = QDir(QCoreApplication::applicationDirPath()).filePath("accounts.json");
    QString path = QFileDialog::getOpenFileName(this, "Import accounts and profiles", defaultPath, "(*.json)");

    if (path.isEmpty())
        return;

    int res = QMessageBox::question(
        this,
        "Import",
        "This will load all accounts and profiles from the file.\n"
        "It may take a while if there are many accounts. Continue?",
        QMessageBox::Yes | QMessageBox::No
    );

    if (res != QMessageBox::Yes)
        return;

    setCursor(Qt::WaitCursor);
    importData(path);
    unsetCursor();
}

bool MainWindow::exportData(const QString &path)
{
    QJsonArray accountsArray;

    for (const GameforgeAccount* acc : gfAccounts) {
        const NostaleAuth* auth = acc->getAuth();

        QJsonObject obj;
        obj["email"] = acc->getEmail();
        obj["password"] = acc->getPassword();
        obj["token"] = auth->getToken();
        obj["identity_path"] = acc->getIdentityPath();
        obj["custom_game_path"] = acc->getcustomClientPath();
        obj["custom_installation_id"] = auth->getInstallationId();
        obj["use_proxy"] = auth->getUseProxy();
        obj["proxy_ip"] = auth->getProxyIp();
        obj["socks_port"] = auth->getSocksPort();
        obj["proxy_username"] = auth->getProxyUsername();
        obj["proxy_password"] = auth->getProxyPassword();
        accountsArray.append(obj);
    }

    QJsonArray profilesArray;

    // Skip the default profile (index 0); it is auto-populated from the accounts.
    for (int i = 1; i < profiles.size(); ++i) {
        Profile* profile = profiles[i];

        QJsonObject profileObj;
        profileObj["profile_name"] = profile->getProfileName();

        QJsonArray gameAccountsArray;
        for (const GameAccount& ga : profile->getAccounts()) {
            if (ga.getGfAcc() == nullptr)
                continue;

            QJsonObject gaObj;
            gaObj["email"] = ga.getGfAcc()->getEmail();
            gaObj["account_name"] = ga.getName();
            gaObj["id"] = ga.getId();
            gaObj["display_name"] = ga.getDisplayName();
            gaObj["server_location"] = ga.getServerLocation();
            gaObj["server"] = ga.getServer();
            gaObj["channel"] = ga.getChannel();
            gaObj["character"] = ga.getSlot();
            gaObj["auto_login"] = ga.getAutoLogin();
            gameAccountsArray.append(gaObj);
        }
        profileObj["game_accounts"] = gameAccountsArray;
        profilesArray.append(profileObj);
    }

    QJsonObject root;
    root["version"] = 1;
    root["generated_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    root["gameforge_accounts"] = accountsArray;
    root["profiles"] = profilesArray;

    QFile out(path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    out.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    out.close();
    return true;
}

QString MainWindow::resolveImportedCustomClientPath(const QString &originalCustomPath)
{
    const QString original = originalCustomPath.trimmed();
    if (original.isEmpty())
        return QString();

    // The exported machine path most likely does not exist here. Keep only the
    // exe file name and derive a local copy inside the current nostale directory.
    const QString fileName = QFileInfo(original).fileName();
    if (fileName.isEmpty())
        return QString();

    const QString nostalePath = settingsDialog->getGameClientPath();
    if (nostalePath.isEmpty())
        return QString();

    const QFileInfo nostaleInfo(nostalePath);
    const QString nostaleDir = nostaleInfo.absolutePath();
    if (nostaleDir.isEmpty())
        return QString();

    // Forward slashes are required by openClient(), which splits on '/'.
    const QString newPath = QDir(nostaleDir).filePath(fileName);

    // If the derived path is the base client itself, no copy is required.
    if (QFileInfo(newPath).absoluteFilePath().compare(nostaleInfo.absoluteFilePath(), Qt::CaseInsensitive) == 0) {
        return newPath;
    }

    if (!QFile::exists(newPath)) {
        if (!QFile::exists(nostalePath)) {
            return QString();
        }
        QFile::copy(nostalePath, newPath);
    }

    return newPath;
}

bool MainWindow::importData(const QString &path)
{
    QFile in(path);
    if (!in.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Import", "Could not open the selected file.");
        return false;
    }

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(in.readAll(), &error);
    in.close();

    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::critical(this, "Import", "The selected file is not a valid Gfless export.");
        return false;
    }

    const QJsonObject root = doc.object();
    const QJsonArray accountsArray = root.value("gameforge_accounts").toArray();
    const QJsonArray profilesArray = root.value("profiles").toArray();

    QSet<QString> existingEmails;
    for (GameforgeAccount* acc : gfAccounts) {
        existingEmails.insert(acc->getEmail());
    }

    int importedAccounts = 0;
    int skippedAccounts = 0;

    for (const QJsonValue& value : accountsArray) {
        const QJsonObject obj = value.toObject();
        const QString email = obj.value("email").toString().trimmed();

        if (email.isEmpty() || existingEmails.contains(email)) {
            ++skippedAccounts;
            continue;
        }

        const QString password = obj.value("password").toString();
        const QString token = obj.value("token").toString();
        const QString identityPath = obj.value("identity_path").toString().trimmed();
        const QString installationId = obj.value("custom_installation_id").toString().trimmed();
        const QString customGamePath = resolveImportedCustomClientPath(obj.value("custom_game_path").toString());
        const bool useProxy = obj.value("use_proxy").toBool();
        const QString proxyIp = obj.value("proxy_ip").toString().trimmed();
        const QString socksPort = obj.value("socks_port").toString().trimmed();
        const QString proxyUsername = obj.value("proxy_username").toString();
        const QString proxyPassword = obj.value("proxy_password").toString();

        ui->statusbar->showMessage("Importing account " + email + "...", 5000);
        QCoreApplication::processEvents();

        const int before = gfAccounts.size();

        if (!token.isEmpty()) {
            addGameforgeAccount(email, password, token, identityPath, installationId, customGamePath, proxyIp, socksPort, proxyUsername, proxyPassword, useProxy);
        }
        else {
            addGameforgeAccount(email, password, identityPath, installationId, customGamePath, proxyIp, socksPort, proxyUsername, proxyPassword, useProxy);
        }

        if (gfAccounts.size() > before) {
            existingEmails.insert(email);
            ++importedAccounts;
        }
        else {
            ++skippedAccounts;
        }
    }

    int importedProfiles = 0;

    for (const QJsonValue& value : profilesArray) {
        const QJsonObject profileObj = value.toObject();
        const QString profileName = profileObj.value("profile_name").toString().trimmed();

        if (profileName.isEmpty())
            continue;

        Profile* profile = nullptr;
        for (int i = 1; i < profiles.size(); ++i) {
            if (profiles[i]->getProfileName() == profileName) {
                profile = profiles[i];
                break;
            }
        }

        if (profile == nullptr) {
            profile = new Profile(profileName, this);
            profiles.push_back(profile);
            ui->profileComboBox->addItem(profileName);
            ++importedProfiles;
        }

        const QJsonArray gameAccountsArray = profileObj.value("game_accounts").toArray();
        for (const QJsonValue& gaValue : gameAccountsArray) {
            const QJsonObject gaObj = gaValue.toObject();
            const QString email = gaObj.value("email").toString();

            GameforgeAccount* gfacc = nullptr;
            for (GameforgeAccount* candidate : gfAccounts) {
                if (candidate->getEmail() == email) {
                    gfacc = candidate;
                    break;
                }
            }

            if (gfacc == nullptr)
                continue;

            GameAccount gameAcc(
                gfacc,
                gaObj.value("account_name").toString(),
                gaObj.value("id").toString(),
                gaObj.value("display_name").toString(),
                gaObj.value("server_location").toInt(),
                gaObj.value("server").toInt(),
                gaObj.value("channel").toInt(),
                gaObj.value("character").toInt(),
                gaObj.value("auto_login").toBool()
            );

            profile->addAccount(gameAcc);
        }
    }

    writeAccountIpsJson();
    saveSettings();
#ifdef NO_PROXY_MODE
    syncProxifierProfile();
#endif
    updateAllGameforgeAccountVisuals();
    displayProfile(ui->profileComboBox->currentIndex());
    on_gameforgeAccountComboBox_currentIndexChanged(ui->gameforgeAccountComboBox->currentIndex());
    ui->statusbar->clearMessage();

    QMessageBox::information(
        this,
        "Import",
        QString("Imported %1 account(s), skipped %2.\nImported %3 new profile(s).")
            .arg(importedAccounts)
            .arg(skippedAccounts)
            .arg(importedProfiles)
    );

    return true;
}

void MainWindow::on_actionIdentity_generator_triggered()
{
    IdentityDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        for (auto* acc : gfAccounts) {
            acc->refreshIdentity();
        }
    }
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

    QQueue<int> accountIndexes;

    for (int i = 0; i < ui->accountsListWidget->selectedItems().count(); ++i) {
        int row = ui->accountsListWidget->row(ui->accountsListWidget->selectedItems().at(i));
        accountIndexes.push_back(row);
    }

    openAccount(profile, accountIndexes);
}


void MainWindow::on_gameforgeAccountComboBox_currentIndexChanged(int index)
{
    if (index < 0)
        return;

    QString tooltipText;

    GameforgeAccount* gf = gfAccounts.at(index);

    if (gf->getAuth()->isProxyActive()) {
        tooltipText += "Proxy IP: " + gf->getAuth()->getProxyIp() + "\n";
        tooltipText += "Proxy port: " + gf->getAuth()->getSocksPort();
    }
    else {
        if (gf->getAuth()->getUseProxy() && !useProxiesGlobally) {
            tooltipText += "No proxy (global toggle)";
        }
        else {
            tooltipText += "No proxy";
        }
    }

    if (gf->getcustomClientPath().isEmpty()) {
        tooltipText += "\nUsing default nostale client";
    }
    else {
        tooltipText += "\nCustom client: " + gf->getcustomClientPath().right(gf->getcustomClientPath().size() - gf->getcustomClientPath().lastIndexOf("/") - 1);
    }

    tooltipText += "\nIdentity: ";
    tooltipText += (gf->getIdentityPath().isEmpty() ? "default" : gf->getIdentityPath());

    tooltipText += "\nInstallation ID: ";
    tooltipText += (gf->getAuth()->getInstallationId().isEmpty() ? "default" : gf->getAuth()->getInstallationId());

    ui->gameforgeAccountComboBox->setToolTip(tooltipText);
}


void MainWindow::on_createGameAccButton_clicked()
{
    CreateGameAccountDialog dialog(gfAccounts, settingsDialog->getGameLanguage(), this);
    if (dialog.exec() == QDialog::Accepted) {
        QJsonValue accInfo = dialog.getNewAccountInfo();

        QString displayName = accInfo["displayName"].toString();
        QString id = accInfo["accountId"].toString();

        GameAccount gameAcc(dialog.getGfAcc(), displayName, id, displayName, defaultServerLocation, defaultServer, defaultChannel, defaultCharacter, defaultAutoLogin);
        profiles.first()->addAccount(gameAcc);
        profiles.first()->sort();

        displayProfile(ui->profileComboBox->currentIndex());
    }
}


void MainWindow::on_repairButton_clicked()
{
    updateGame();
}

void MainWindow::setupProxyControls()
{
    QWidget* cornerWidget = new QWidget(this);
    QHBoxLayout* layout = new QHBoxLayout(cornerWidget);
    layout->setContentsMargins(4, 0, 4, 0);
    layout->setSpacing(4);

    toggleProxyModeButton = new QToolButton(cornerWidget);
    toggleProxyModeButton->setCheckable(true);
    toggleProxyModeButton->setChecked(true);

    patchNewProxiesButton = new QToolButton(cornerWidget);
    patchNewProxiesButton->setText("Patch new proxies");

    layout->addWidget(toggleProxyModeButton);
    layout->addWidget(patchNewProxiesButton);

    ui->menubar->setCornerWidget(cornerWidget, Qt::TopRightCorner);

    connect(toggleProxyModeButton, &QToolButton::toggled, this, [&](bool checked) {
        useProxiesGlobally = checked;
        applyGlobalProxyMode();
        updateProxyModeButtonText();
        on_gameforgeAccountComboBox_currentIndexChanged(ui->gameforgeAccountComboBox->currentIndex());
    });

    connect(patchNewProxiesButton, &QToolButton::clicked, this, [&]() {
        // Persist current UI/account state to registry and JSON before patching.
        saveSettings();
        int patched = patchNewProxiesFromJson();
#ifdef NO_PROXY_MODE
        // Always refresh Proxifier profile on explicit patch action.
        syncProxifierProfile();
#endif
        if (patched <= 0) {
            QMessageBox::information(this, "Patch new proxies", "No account entries were patched from accountIPS.json.\nProxifier profile was still refreshed.");
            return;
        }

        QMessageBox::information(
            this,
            "Patch new proxies",
            "Patched " + QString::number(patched) + " account(s) from accountIPS.json."
        );
    });

    updateProxyModeButtonText();
}

void MainWindow::applyGlobalProxyMode()
{
    for (GameforgeAccount* acc : gfAccounts) {
        acc->getAuth()->setForceNoProxy(!useProxiesGlobally);
    }
}

void MainWindow::updateProxyModeButtonText()
{
    if (!toggleProxyModeButton) {
        return;
    }

    toggleProxyModeButton->blockSignals(true);
    toggleProxyModeButton->setChecked(useProxiesGlobally);
    toggleProxyModeButton->setText(useProxiesGlobally ? "Use proxies" : "No use proxies");
    toggleProxyModeButton->blockSignals(false);
}

void MainWindow::updateGameforgeAccountVisual(int index)
{
    if (index < 0 || index >= gfAccounts.size() || index >= ui->gameforgeAccountComboBox->count()) {
        return;
    }

    const bool hasProxy = gfAccounts.at(index)->getAuth()->getUseProxy();
    const QColor color = hasProxy ? QColor(0, 140, 0) : QColor(180, 30, 30);
    ui->gameforgeAccountComboBox->setItemData(index, QBrush(color), Qt::ForegroundRole);
}

void MainWindow::updateAllGameforgeAccountVisuals()
{
    for (int i = 0; i < gfAccounts.size(); ++i) {
        updateGameforgeAccountVisual(i);
    }
}

QString MainWindow::resolveProxifierProfilePath() const
{
    QString appData = qEnvironmentVariable("APPDATA").trimmed();
    if (appData.isEmpty()) {
        appData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    }
    if (appData.isEmpty()) {
        return QString();
    }

    QDir profilesDir(QDir(appData).filePath("Proxifier4/Profiles"));
    if (!profilesDir.exists()) {
        profilesDir.mkpath(".");
    }

    return profilesDir.filePath("GFLESSCLIENT.ppx");
}

QDomDocument MainWindow::createDefaultProxifierProfile() const
{
    QDomDocument doc;
    const QString xmlTemplate = QStringLiteral(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<ProxifierProfile version="102" platform="Windows" product_id="0" product_minver="400">
	<Options>
		<Resolve>
			<AutoModeDetection enabled="true" />
			<ViaProxy enabled="false" />
			<BlockNonATypes enabled="false" />
			<ExclusionList OnlyFromListMode="false">%ComputerName%; localhost; *.local</ExclusionList>
			<DnsUdpMode>0</DnsUdpMode>
		</Resolve>
		<Encryption mode="disabled" />
		<ConnectionLoopDetection enabled="true" resolve="true" />
		<Udp mode="mode_bypass" />
		<LeakPreventionMode enabled="false" />
		<ProcessOtherUsers enabled="false" />
		<ProcessServices enabled="false" />
		<HandleDirectConnections enabled="false" />
		<HttpProxiesSupport enabled="false" />
	</Options>
	<ProxyList />
	<ChainList />
	<RuleList>
		<Rule enabled="true">
			<Action type="Direct" />
			<Targets>localhost; 127.0.0.1; %ComputerName%; ::1</Targets>
			<Name>Localhost</Name>
		</Rule>
		<Rule enabled="true">
			<Action type="Direct" />
			<Name>Default</Name>
		</Rule>
	</RuleList>
</ProxifierProfile>
)");
    if (!doc.setContent(xmlTemplate)) {
        doc.clear();
    }

    return doc;
}

void MainWindow::syncProxifierProfile()
{
    const QString profilePath = resolveProxifierProfilePath();
    if (profilePath.isEmpty()) {
        return;
    }

    const QFileInfo targetInfo(profilePath);
    const QDir profileDir(targetInfo.absolutePath());
    const QStringList candidatePaths = {
        profilePath,
        profileDir.filePath("gflessclient.ppx"),
        profileDir.filePath("GFLESSDLL.ppx")
    };

    QDomDocument doc;
    bool parsed = false;
    for (const QString& candidate : candidatePaths) {
        QFile inFile(candidate);
        if (!inFile.exists() || !inFile.open(QIODevice::ReadOnly)) {
            continue;
        }
        parsed = doc.setContent(&inFile);
        inFile.close();
        if (parsed) {
            break;
        }
    }

    QDomElement root = doc.documentElement();
    if (!parsed || root.isNull() || root.tagName() != "ProxifierProfile") {
        doc = createDefaultProxifierProfile();
        root = doc.documentElement();
    } else {
        // Rebase to a known-good Proxifier schema and preserve sections from the existing profile.
        QDomDocument sanitized = createDefaultProxifierProfile();
        QDomElement sanitizedRoot = sanitized.documentElement();
        auto copySection = [&](const QString& tagName) {
            const QDomElement existing = root.firstChildElement(tagName);
            if (existing.isNull()) {
                return;
            }
            const QDomElement target = sanitizedRoot.firstChildElement(tagName);
            const QDomNode imported = sanitized.importNode(existing, true);
            if (target.isNull()) {
                sanitizedRoot.appendChild(imported);
            } else {
                sanitizedRoot.replaceChild(imported, target);
            }
        };
        copySection("ProxyList");
        copySection("ChainList");
        copySection("RuleList");
        doc = sanitized;
        root = doc.documentElement();
    }

    QDomElement proxyList = root.firstChildElement("ProxyList");
    if (proxyList.isNull()) {
        proxyList = doc.createElement("ProxyList");
        root.appendChild(proxyList);
    }

    QDomElement ruleList = root.firstChildElement("RuleList");
    if (ruleList.isNull()) {
        ruleList = doc.createElement("RuleList");
        root.appendChild(ruleList);
    }

    auto normalizedPath = [](const QString& path) {
        return QDir::toNativeSeparators(path).trimmed().toLower();
    };

    auto splitApplications = [&](const QString& applicationsText) {
        QStringList parts = applicationsText.split(';', Qt::SkipEmptyParts);
        for (QString& part : parts) {
            part = part.trimmed();
            if (part.startsWith("\"") && part.endsWith("\"") && part.size() >= 2) {
                part = part.mid(1, part.size() - 2);
            }
            part = normalizedPath(part);
        }
        return parts;
    };

    QMap<QString, int> proxyKeyToId;
    int maxProxyId = 99;

    for (QDomElement proxy = proxyList.firstChildElement("Proxy"); !proxy.isNull(); proxy = proxy.nextSiblingElement("Proxy")) {
        bool ok = false;
        const int id = proxy.attribute("id").toInt(&ok);
        if (ok && id > maxProxyId) {
            maxProxyId = id;
        }

        const QString address = proxy.firstChildElement("Address").text().trimmed();
        const QString port = proxy.firstChildElement("Port").text().trimmed();
        QString username;
        QString password;
        QDomElement auth = proxy.firstChildElement("Authentication");
        if (!auth.isNull() && auth.attribute("enabled").toLower() == "true") {
            username = auth.firstChildElement("Username").text();
            password = auth.firstChildElement("Password").text();
        }

        const QString key = address + "|" + port + "|" + username + "|" + password;
        if (!address.isEmpty() && !port.isEmpty() && ok) {
            proxyKeyToId[key] = id;
        }
    }

    struct ProxyAccountEntry {
        QString customPath;
        QString proxyIp;
        QString proxyPort;
        QString proxyUser;
        QString proxyPass;
    };

    QVector<ProxyAccountEntry> patchEntries;
    auto addPatchEntry = [&](const QString& customPathRaw,
                             const QString& proxyIpRaw,
                             const QString& proxyPortRaw,
                             const QString& proxyUserRaw,
                             const QString& proxyPassRaw,
                             bool useProxy) {
        if (!useProxy) {
            return;
        }

        QString customPath = customPathRaw.trimmed();
        QString proxyIp = proxyIpRaw.trimmed();
        QString proxyPort = proxyPortRaw.trimmed();
        QString proxyUser = proxyUserRaw.trimmed();
        QString proxyPass = proxyPassRaw;

        if (customPath.startsWith("\"") && customPath.endsWith("\"") && customPath.size() >= 2) {
            customPath = customPath.mid(1, customPath.size() - 2).trimmed();
        }

        if (proxyIp.isEmpty() || proxyPort.isEmpty()) {
            return;
        }

        patchEntries.push_back({customPath, proxyIp, proxyPort, proxyUser, proxyPass});
    };

    for (GameforgeAccount* acc : gfAccounts) {
        if (!acc) {
            continue;
        }

        const NostaleAuth* auth = acc->getAuth();
        if (!auth) {
            continue;
        }

        addPatchEntry(
            acc->getcustomClientPath(),
            auth->getProxyIp(),
            auth->getSocksPort(),
            auth->getProxyUsername(),
            auth->getProxyPassword(),
            auth->getUseProxy()
        );
    }

    // Merge with persisted accounts from registry to avoid losing rules/proxies that are not currently loaded in memory.
    {
        QSettings settings;
        settings.beginGroup("Gameforge Accounts");
        const int numAccs = settings.beginReadArray("GF accounts data");
        for (int i = 0; i < numAccs; ++i) {
            settings.setArrayIndex(i);
            addPatchEntry(
                settings.value("custom_client", "").toString(),
                settings.value("proxy_ip", "").toString(),
                settings.value("socks_port", "").toString(),
                settings.value("proxy_username", "").toString(),
                settings.value("proxy_password", "").toString(),
                settings.value("use_proxy", false).toBool()
            );
        }
        settings.endArray();
        settings.endGroup();
    }

    QSet<QString> managedPaths;

    for (const ProxyAccountEntry& entry : patchEntries) {
        const QString proxyIp = entry.proxyIp;
        const QString proxyPort = entry.proxyPort;
        const QString proxyUser = entry.proxyUser;
        const QString proxyPass = entry.proxyPass;

        const QString proxyKey = proxyIp + "|" + proxyPort + "|" + proxyUser + "|" + proxyPass;
        int proxyId = proxyKeyToId.value(proxyKey, -1);

        if (proxyId < 0) {
            proxyId = ++maxProxyId;
            QDomElement proxyNode = doc.createElement("Proxy");
            proxyNode.setAttribute("id", QString::number(proxyId));
            proxyNode.setAttribute("type", "SOCKS5");

            QDomElement authNode = doc.createElement("Authentication");
            const bool authEnabled = (!proxyUser.isEmpty() || !proxyPass.isEmpty());
            authNode.setAttribute("enabled", authEnabled ? "true" : "false");
            if (authEnabled) {
                QDomElement passNode = doc.createElement("Password");
                passNode.appendChild(doc.createTextNode(proxyPass));
                authNode.appendChild(passNode);
                QDomElement userNode = doc.createElement("Username");
                userNode.appendChild(doc.createTextNode(proxyUser));
                authNode.appendChild(userNode);
            }
            proxyNode.appendChild(authNode);

            QDomElement optionsNode = doc.createElement("Options");
            optionsNode.appendChild(doc.createTextNode("48"));
            proxyNode.appendChild(optionsNode);

            QDomElement portNode = doc.createElement("Port");
            portNode.appendChild(doc.createTextNode(proxyPort));
            proxyNode.appendChild(portNode);

            QDomElement addressNode = doc.createElement("Address");
            addressNode.appendChild(doc.createTextNode(proxyIp));
            proxyNode.appendChild(addressNode);

            proxyList.appendChild(proxyNode);
            proxyKeyToId[proxyKey] = proxyId;
        }

        const QString customPath = entry.customPath;
        if (customPath.isEmpty()) {
            continue;
        }

        const QString appPathNormalized = normalizedPath(customPath);
        const QString appPathForRule = "\"" + QDir::toNativeSeparators(customPath) + "\"";
        managedPaths.insert(appPathNormalized);

        QDomElement matchedRule;
        QList<QDomElement> duplicates;
        for (QDomElement rule = ruleList.firstChildElement("Rule"); !rule.isNull(); rule = rule.nextSiblingElement("Rule")) {
            QDomElement applications = rule.firstChildElement("Applications");
            if (applications.isNull()) {
                continue;
            }

            const QStringList appEntries = splitApplications(applications.text());
            if (!appEntries.contains(appPathNormalized)) {
                continue;
            }

            if (matchedRule.isNull()) {
                matchedRule = rule;
            } else {
                duplicates.push_back(rule);
            }
        }

        for (QDomElement duplicate : duplicates) {
            ruleList.removeChild(duplicate);
        }

        if (matchedRule.isNull()) {
            matchedRule = doc.createElement("Rule");
            matchedRule.setAttribute("enabled", "true");
            QDomElement defaultRule;
            for (QDomElement rule = ruleList.firstChildElement("Rule"); !rule.isNull(); rule = rule.nextSiblingElement("Rule")) {
                if (rule.firstChildElement("Name").text().trimmed().compare("Default", Qt::CaseInsensitive) == 0) {
                    defaultRule = rule;
                    break;
                }
            }
            if (!defaultRule.isNull()) {
                ruleList.insertBefore(matchedRule, defaultRule);
            } else {
                ruleList.appendChild(matchedRule);
            }
        }

        QDomElement action = matchedRule.firstChildElement("Action");
        if (action.isNull()) {
            action = doc.createElement("Action");
            matchedRule.appendChild(action);
        }
        action.setAttribute("type", "Proxy");
        while (action.firstChild().isNull() == false) {
            action.removeChild(action.firstChild());
        }
        action.appendChild(doc.createTextNode(QString::number(proxyId)));

        QDomElement applications = matchedRule.firstChildElement("Applications");
        if (applications.isNull()) {
            applications = doc.createElement("Applications");
            matchedRule.appendChild(applications);
        }
        while (applications.firstChild().isNull() == false) {
            applications.removeChild(applications.firstChild());
        }
        applications.appendChild(doc.createTextNode(appPathForRule));

        QDomElement name = matchedRule.firstChildElement("Name");
        if (name.isNull()) {
            name = doc.createElement("Name");
            matchedRule.appendChild(name);
        }
        while (name.firstChild().isNull() == false) {
            name.removeChild(name.firstChild());
        }
        name.appendChild(doc.createTextNode(QFileInfo(customPath).fileName()));

        matchedRule.setAttribute("enabled", "true");
    }

    // Disable managed rules when the related account no longer has custom path/proxy.
    for (QDomElement rule = ruleList.firstChildElement("Rule"); !rule.isNull(); rule = rule.nextSiblingElement("Rule")) {
        QDomElement applications = rule.firstChildElement("Applications");
        if (applications.isNull()) {
            continue;
        }
        const QStringList appEntries = splitApplications(applications.text());
        if (appEntries.size() != 1) {
            continue;
        }
        const QString appPath = appEntries.first();
        if (managedPaths.contains(appPath)) {
            continue;
        }

        const QString ruleName = rule.firstChildElement("Name").text().trimmed();
        if (ruleName.endsWith(".exe", Qt::CaseInsensitive) && appPath.contains("\\") && appPath.endsWith(".exe")) {
            QDomElement action = rule.firstChildElement("Action");
            if (!action.isNull() && action.attribute("type").compare("Proxy", Qt::CaseInsensitive) == 0) {
                rule.setAttribute("enabled", "false");
            }
        }
    }

    QDir().mkpath(targetInfo.absolutePath());
    const QStringList outputPaths = {
        profilePath,
        profileDir.filePath("gflessclient.ppx"),
        profileDir.filePath("GFLESSDLL.ppx")
    };

    bool wroteAny = false;
    QString xmlText = doc.toString(1);
    const QString forcedXmlDeclaration = QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>");
    if (xmlText.startsWith("<?xml")) {
        const int declarationEnd = xmlText.indexOf("?>");
        if (declarationEnd >= 0) {
            xmlText = forcedXmlDeclaration + xmlText.mid(declarationEnd + 2);
        } else {
            xmlText.prepend(forcedXmlDeclaration + "\n");
        }
    } else {
        xmlText.prepend(forcedXmlDeclaration + "\n");
    }

    for (const QString& outputPath : outputPaths) {
        QFile outFile(outputPath);
        if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            continue;
        }
        QTextStream out(&outFile);
        out.setCodec("UTF-8");
        out << xmlText;
        outFile.close();
        wroteAny = true;
    }

    if (!wroteAny) {
        return;
    }
}

QString MainWindow::getAccountIpsJsonPath() const
{
    return QDir(QCoreApplication::applicationDirPath()).filePath("accountIPS.json");
}

void MainWindow::writeAccountIpsJson() const
{
    QJsonArray accounts;

    for (const GameforgeAccount* acc : gfAccounts) {
        const NostaleAuth* auth = acc->getAuth();

        QJsonObject obj;
        obj["email"] = acc->getEmail();
        obj["use_proxy"] = auth->getUseProxy();
        obj["proxy_ip"] = auth->getProxyIp();
        obj["socks_port"] = auth->getSocksPort();
        obj["proxy_username"] = auth->getProxyUsername();
        obj["proxy_password"] = auth->getProxyPassword();
        obj["identity_path"] = acc->getIdentityPath();
        obj["custom_client"] = acc->getcustomClientPath();
        obj["custom_game_path"] = acc->getcustomClientPath();
        obj["installation_id"] = auth->getInstallationId();
        obj["custom_installation_id"] = auth->getInstallationId();
        accounts.append(obj);
    }

    QJsonObject root;
    root["generated_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    root["accounts"] = accounts;

    QFile out(getAccountIpsJsonPath());
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }

    out.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    out.close();
}

int MainWindow::patchNewProxiesFromJson()
{
    QFile input(getAccountIpsJsonPath());
    if (!input.open(QIODevice::ReadOnly)) {
        return 0;
    }

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(input.readAll(), &error);
    input.close();

    if (error.error != QJsonParseError::NoError || doc.isNull()) {
        return 0;
    }

    QJsonArray sourceAccounts;

    if (doc.isArray()) {
        sourceAccounts = doc.array();
    } else if (doc.isObject()) {
        sourceAccounts = doc.object().value("accounts").toArray();
    }

    if (sourceAccounts.isEmpty()) {
        return 0;
    }

    QMap<QString, QJsonObject> byEmail;
    for (const QJsonValue& value : sourceAccounts) {
        const QJsonObject obj = value.toObject();
        const QString email = obj.value("email").toString().trimmed();
        if (email.isEmpty()) {
            continue;
        }
        byEmail[email] = obj;
    }

    int patched = 0;
    for (GameforgeAccount* acc : gfAccounts) {
        if (!byEmail.contains(acc->getEmail())) {
            continue;
        }

        QJsonObject obj = byEmail.value(acc->getEmail());
        NostaleAuth* auth = acc->getAuth();

        QString ip = auth->getProxyIp();
        QString port = auth->getSocksPort();
        QString user = auth->getProxyUsername();
        QString pass = auth->getProxyPassword();
        QString identityPath = acc->getIdentityPath();
        QString installationId = auth->getInstallationId();
        QString customClientPath = acc->getcustomClientPath();
        bool useProxy = auth->getUseProxy();

        if (obj.contains("proxy_ip")) {
            ip = obj.value("proxy_ip").toString().trimmed();
        }
        if (obj.contains("socks_port")) {
            port = obj.value("socks_port").toString().trimmed();
        }
        if (obj.contains("proxy_username")) {
            user = obj.value("proxy_username").toString();
        }
        if (obj.contains("proxy_password")) {
            pass = obj.value("proxy_password").toString();
        }
        if (obj.contains("identity_path")) {
            identityPath = obj.value("identity_path").toString().trimmed();
        }
        if (obj.contains("installation_id")) {
            installationId = obj.value("installation_id").toString().trimmed();
        } else if (obj.contains("custom_installation_id")) {
            installationId = obj.value("custom_installation_id").toString().trimmed();
        }
        if (obj.contains("custom_client")) {
            customClientPath = obj.value("custom_client").toString().trimmed();
        } else if (obj.contains("custom_game_path")) {
            customClientPath = obj.value("custom_game_path").toString().trimmed();
        }

        if (obj.contains("use_proxy")) {
            const QJsonValue useProxyValue = obj.value("use_proxy");
            if (useProxyValue.isBool()) {
                useProxy = useProxyValue.toBool();
            } else if (useProxyValue.isString()) {
                const QString raw = useProxyValue.toString().trimmed().toLower();
                if (!raw.isEmpty()) {
                    useProxy = (raw == "true" || raw == "1" || raw == "yes");
                }
            } else if (useProxyValue.isDouble()) {
                useProxy = (useProxyValue.toInt() != 0);
            }
        }

        acc->setAdvancedConfig(identityPath, installationId, customClientPath);
        acc->setProxyConfig(useProxy, ip, port, user, pass);
        acc->getAuth()->setForceNoProxy(!useProxiesGlobally);
        patched++;
    }

    if (patched > 0) {
        saveSettings();
        updateAllGameforgeAccountVisuals();
        displayProfile(ui->profileComboBox->currentIndex());
        on_gameforgeAccountComboBox_currentIndexChanged(ui->gameforgeAccountComboBox->currentIndex());
    }

    return patched;
}

void MainWindow::openAccount(const Profile *profile, QQueue<int> accountIndexes)
{
    if (accountIndexes.empty()) {
        return;
    }

    int row = accountIndexes.front();
    accountIndexes.pop_front();

    int openInterval = settingsDialog->getOpenInterval();
    QString gamePath = settingsDialog->getGameClientPath();
    int gameLang = settingsDialog->getGameLanguage();

    const GameAccount& gameAccount = profile->getAccounts().at(row);

    ui->statusbar->showMessage("Trying to open account " + gameAccount.getName(), 10000);

    QString token = gameAccount.getGfAcc()->getToken(gameAccount.getId());

    if (token.isEmpty()) {
        QString detail = gameAccount.getGfAcc()->getAuth()->getLastError();
        if (detail.isEmpty()) {
            detail = "Couldn't get token";
        }
        ui->statusbar->showMessage(detail + " (" + gameAccount.getName() + ")", 12000);
    }
    else {
        DWORD pid = 0;

        QString customPath = gameAccount.getGfAcc()->getcustomClientPath();

        if (gflessClient->openClient(gameAccount.getName(), token, customPath.isEmpty() ? gamePath : customPath, gameLang, pid)) {
            QString msg = "Launched game with PID " + QString::number(pid);
            ui->statusbar->showMessage(msg, 10000);
            processAccounts.insert(pid, gameAccount);
        }
        else {
            ui->statusbar->showMessage("Failed to launch game for account " + gameAccount.getName(), 10000);
        }
    }

    QTimer::singleShot(openInterval * 1000, this, [=]() {
        openAccount(profile, accountIndexes);
    });
}
