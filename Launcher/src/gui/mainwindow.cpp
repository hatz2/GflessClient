#include "mainwindow.h"
#include "addaccountdialog.h"
#include "addprofileaccountdialog.h"
#include "addprofiledialog.h"
#include "captchadialog.h"
#include "identitydialog.h"
#include "editmultipleprofileaccountsdialog.h"
#include "gameupdatedialog.h"
#include "creategameaccountdialog.h"
#include "otpdialog.h"
#include "powcaptchasolver.h"
#include "credentialprotection.h"
#include <QInputDialog>
#include <QScopedValueRollback>
#include <QSignalBlocker>
#include "ui_mainwindow.h"

namespace {
const QString accountsArray = QStringLiteral("GF accounts data");
const QString protectedTokenKey = QStringLiteral("token_dpapi");
const QString protectedProxyKey = QStringLiteral("proxy_password_dpapi");

void removeStaleCredentials(QSettings &settings, qsizetype accountCount)
{
    const QString prefix = accountsArray + '/';
    for (const QString &key : settings.allKeys()) {
        if (!key.startsWith(prefix, Qt::CaseInsensitive))
            continue;

        const QString field = key.section('/', -1).toLower();
        const int entry = key.section('/', 1, 1).toInt();
        const bool oldCredential = field == "password" || field == "token" || field == "proxy_password";
        const bool protectedCredential = field == protectedTokenKey || field == protectedProxyKey;
        const bool unusedCredential = protectedCredential && (entry < 1 || entry > accountCount);
        if (oldCredential || unusedCredential)
            settings.remove(key);
    }
}

bool saveProtectedCredential(QSettings &settings, const QString &key, const QString &credential)
{
    const QByteArray encrypted = CredentialProtection::protect(credential);
    if (encrypted.isEmpty())
        settings.remove(key);
    else
        settings.setValue(key, encrypted);

    return credential.isEmpty() || !encrypted.isEmpty();
}
}

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
    const QScopedValueRollback accountOperation(accountOperationInProgress, true);
    QSettings settings;

    settings.beginGroup("MainWindow");
    restoreGeometry(settings.value("geometry").toByteArray());
    settingsDialog->setGameClientPath(settings.value("nostale path").toString());
    settingsDialog->setProfilesPath(settings.value("profiles path").toString());
    settingsDialog->setOpenInterval(settings.value("open interval", 10).toInt());
    settingsDialog->setGameLanguage(settings.value("game language", 0).toInt());
    settingsDialog->setTheme(settings.value("theme", ThemeDefault).toInt());
    settingsDialog->setThemeComboBox(settings.value("theme", ThemeDefault).toInt());
    settingsDialog->setDisabledNosmall(settings.value("disable_nosmall", false).toBool());
    settingsDialog->setCheckUpdates(settings.value("check_updates", true).toBool());

    defaultAutoLogin = settings.value("default_autologin", false).toBool();
    defaultServerLocation = settings.value("default_serverlocation", 0).toInt();
    defaultServer = settings.value("default_server", 0).toInt();
    defaultChannel = settings.value("default_channel", 0).toInt();
    defaultCharacter = settings.value("default_character", 0).toInt();

    settings.endGroup();

    settings.setFallbacksEnabled(false);
    settings.beginGroup("Gameforge Accounts");
    removeStaleCredentials(settings, settings.value(accountsArray + "/size", 0).toInt());
    const int numAccs = settings.beginReadArray(accountsArray);

    for (int i = 0; i < numAccs; ++i) {
        settings.setArrayIndex(i);

        QString email = settings.value("email", "").toString();
        QString identity = settings.value("identity_path", "").toString();
        QString installationId = settings.value("installation_id", "").toString();
        QString proxyIp = settings.value("proxy_ip", "").toString();
        QString socksPort = settings.value("socks_port", "").toString();
        QString proxyUsername = settings.value("proxy_username", "").toString();
        QString customClientPath = settings.value("custom_client", "").toString();
        bool useProxy = settings.value("use_proxy", false).toBool();

        auto *account = new GameforgeAccount(email, identity, installationId, customClientPath, useProxy, proxyIp,
                                            socksPort, proxyUsername, this);
        account->setRememberSession(settings.value("remember_session", false).toBool());
        if (account->getRememberSession()) {
            account->setSessionToken(CredentialProtection::unprotect(settings.value(protectedTokenKey).toByteArray()));
            if (account->needsProxyPassword()) {
                const QByteArray encrypted = settings.value(protectedProxyKey).toByteArray();
                const QString password = CredentialProtection::unprotect(encrypted);
                if (!password.isEmpty())
                    account->setProxyPassword(password);
            }
        }
        if (!account->hasSession())
            settings.remove(protectedTokenKey);
        if (account->getAuth()->getProxyPassword().isEmpty())
            settings.remove(protectedProxyKey);

        gfAccounts.push_back(account);
        ui->gameforgeAccountComboBox->addItem(email);
    }

    settings.endArray();
    settings.endGroup();
    settings.sync();
    if (settings.status() != QSettings::NoError)
        QMessageBox::warning(this, tr("Session storage"), tr("Windows could not clean up the saved credentials."));

    for (auto *account : gfAccounts) {
        if (account->hasSession() && !account->needsProxyPassword())
            refreshGameAccounts(account);
    }
    updateSessionControls();
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
    settings.setValue("disable_nosmall", settingsDialog->getDisabledNosmall());
    settings.setValue("check_updates", settingsDialog->getCheckUpdates());
    settings.setValue("default_autologin", defaultAutoLogin);
    settings.setValue("default_serverlocation", defaultServerLocation);
    settings.setValue("default_server", defaultServer);
    settings.setValue("default_channel", defaultChannel);
    settings.setValue("default_character", defaultCharacter);
    settings.endGroup();

    settings.setFallbacksEnabled(false);
    settings.beginGroup("Gameforge Accounts");
    removeStaleCredentials(settings, gfAccounts.size());
    settings.beginWriteArray(accountsArray, gfAccounts.size());
    bool encryptionFailed = false;
    for (int i = 0; i < gfAccounts.size(); ++i) {
        settings.setArrayIndex(i);
        GameforgeAccount* acc = gfAccounts.at(i);

        const bool rememberSession = acc->getRememberSession();
        const QString token = rememberSession ? acc->getSessionToken() : QString();
        const QString proxyPassword = rememberSession ? acc->getAuth()->getProxyPassword() : QString();
        encryptionFailed |= !saveProtectedCredential(settings, protectedTokenKey, token);
        encryptionFailed |= !saveProtectedCredential(settings, protectedProxyKey, proxyPassword);

        settings.setValue("remember_session", rememberSession);
        settings.setValue("email", acc->getEmail());
        settings.setValue("identity_path", acc->getIdentityPath());
        settings.setValue("installation_id", acc->getAuth()->getInstallationId());
        settings.setValue("custom_client", acc->getcustomClientPath());

        settings.setValue("use_proxy", acc->getAuth()->getUseProxy());
        settings.setValue("proxy_ip", acc->getAuth()->getProxyIp());
        settings.setValue("socks_port", acc->getAuth()->getSocksPort());
        settings.setValue("proxy_username", acc->getAuth()->getProxyUsername());
    }

    settings.endArray();
    settings.endGroup();
    settings.sync();
    if (settings.status() != QSettings::NoError)
        QMessageBox::warning(this, tr("Session storage"), tr("Windows could not update the saved settings."));
    else if (encryptionFailed)
        QMessageBox::warning(this, tr("Session storage"), tr("Some credentials could not be encrypted or saved."));
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

// Handles the challenge Gameforge answered the login with. Returns true when
// it was passed and the login can be retried.
bool MainWindow::solveCaptcha(const QString &gfChallengeId, const NostaleAuth *auth)
{
    auto *netManager = auth->getNetworkManager();
    QJsonObject captchaInfo = CaptchaSolver::getCaptchaInfo(gfChallengeId, netManager);
    QString type = captchaInfo.value("type").toString();
    QString script = captchaInfo.value("script").toString();

    if (type == "gf-image-drop-captcha") {
        CaptchaDialog captcha(gfChallengeId, netManager, this);

        return captcha.exec() == QDialog::Accepted;
    }

    if (type == "gf-pow-captcha") {
        // Nothing to answer: Gameforge's own script runs the check in a browser.
        PowCaptchaSolver solver(gfChallengeId, script, this);

        if (auth->getUseProxy())
            solver.setProxy(auth->getProxyIp(), auth->getSocksPort(), auth->getProxyUsername(),
                            auth->getProxyPassword());

        PowCaptchaSolver::Result result = solver.run(this);

        if (!result.solved) {
            QMessageBox::warning(
                this,
                "Gameforge login check failed",
                "Gameforge's login check did not pass: " + result.detail
                    + "\n\nYou can try to add the account again, or log in once with the Gameforge client or your browser."
            );
        }

        return result.solved;
    }

    QMessageBox::critical(
        this,
        "Unsupported type of captcha",
        "Unsupported type of captcha\n\nType: " + type + "\nScript: " + script
            + "\n\nYou need to resolve it using the gameforge client or your browser."
    );

    return false;
}

bool MainWindow::authenticateAccount(GameforgeAccount *account, const QString &password)
{
    for (;;) {
        bool captcha = false;
        bool wrongCredentials = false;
        QString challengeId;
        if (account->authenticate(password, captcha, challengeId, wrongCredentials))
            return true;

        if (captcha) {
            if (solveCaptcha(challengeId, account->getAuth()))
                continue;
            return false;
        }
        if (account->getOtpRequired()) {
            OtpDialog dialog(this);
            if (dialog.exec() != QDialog::Accepted)
                return false;
            account->setOtpCode(dialog.getOtpCode());
            continue;
        }

        const QString error = wrongCredentials ? tr("Incorrect email or password.") : tr("Authentication failed.");
        QMessageBox::critical(this, tr("Sign in"), error);
        return false;
    }
}

bool MainWindow::refreshGameAccounts(GameforgeAccount *account)
{
    if (!account->updateGameAccounts()) {
        if (!account->hasSession())
            sessionExpired(account);
        return false;
    }

    removeAccountsFromDefaultProfile(account);
    const auto &gameAccounts = account->getGameAccounts();
    for (auto it = gameAccounts.cbegin(); it != gameAccounts.cend(); ++it) {
        GameAccount gameAccount(account, it.value(), it.key(), it.value(), defaultServerLocation, defaultServer,
                                defaultChannel, defaultCharacter, defaultAutoLogin);
        profiles.first()->addAccount(gameAccount);
    }

    displayProfile(ui->profileComboBox->currentIndex());
    return true;
}

void MainWindow::clearAccountSession(GameforgeAccount *account, bool keepProxyPassword)
{
    account->clearSession(keepProxyPassword);
    removeAccountsFromDefaultProfile(account);
    displayProfile(ui->profileComboBox->currentIndex());
    updateSessionControls();
}

void MainWindow::sessionExpired(GameforgeAccount *account)
{
    clearAccountSession(account, account->getRememberSession());
    saveSettings();
    ui->statusbar->showMessage(tr("Session expired. Sign in again as %1.").arg(account->getEmail()), 10000);
}

void MainWindow::removeAccountsFromDefaultProfile(GameforgeAccount *account)
{
    if (profiles.count() <= 0)
        return;

    Profile* defaultProfile = profiles[0];

    const auto &accs = defaultProfile->getAccounts();

    for (int i = accs.count() - 1; i >= 0; --i) {
        if (accs[i].getGfAcc() == account) {
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
    saveSettings();
    event->ignore();
}


void MainWindow::on_addGameforgeAccountButton_clicked()
{
    if (accountOperationInProgress)
        return;

    const QScopedValueRollback accountOperation(accountOperationInProgress, true);
    GameforgeAccount *account;
    {
        AddAccountDialog dialog(this);
        if (dialog.exec() != QDialog::Accepted)
            return;

        account = new GameforgeAccount(dialog.getEmail(), dialog.getIdentityPath(), dialog.getInstallationId(),
                                       dialog.getCustomClientPath(), dialog.getUseProxy(), dialog.getProxyIp(),
                                       dialog.getSocksPort(), dialog.getProxyUsername(), this);
        account->setProxyPassword(dialog.getProxyPassword());
        if (!authenticateAccount(account, dialog.getPassword())) {
            delete account;
            return;
        }
    }

    gfAccounts.push_back(account);
    ui->gameforgeAccountComboBox->addItem(account->getEmail());
    ui->gameforgeAccountComboBox->setCurrentIndex(gfAccounts.size() - 1);
    saveSettings();
    refreshGameAccounts(account);
}

void MainWindow::on_removeGameforgeAccountButton_clicked()
{
    if (accountOperationInProgress || !selectedGameforgeAccount())
        return;

    const QScopedValueRollback accountOperation(accountOperationInProgress, true);
    const int index = ui->gameforgeAccountComboBox->currentIndex();
    const QString message = tr("Remove this account from the launcher? This will not delete your Gameforge account.");
    if (QMessageBox::question(this, tr("Remove account"), message) != QMessageBox::Yes)
        return;

    clearAccountSession(gfAccounts.at(index));
    gfAccounts.removeAt(index);
    ui->gameforgeAccountComboBox->removeItem(index);
    updateSessionControls();
    saveSettings();
}

void MainWindow::on_signInButton_clicked()
{
    auto *account = selectedGameforgeAccount();
    if (accountOperationInProgress || !account)
        return;

    const QScopedValueRollback accountOperation(accountOperationInProgress, true);
    const bool needsProxyPassword = account->needsProxyPassword();
    if (needsProxyPassword) {
        bool accepted = false;
        const QString prompt = tr("Proxy password for %1:").arg(account->getEmail());
        const QString password = QInputDialog::getText(this, tr("Connect"), prompt, QLineEdit::Password, {}, &accepted);
        if (!accepted || password.isEmpty())
            return;
        account->setProxyPassword(password);
    }

    const bool signingIn = !account->hasSession();
    if (signingIn) {
        bool accepted = false;
        const QString prompt = tr("Gameforge password for %1:").arg(account->getEmail());
        const QString title = tr("Sign in");
        const QString password = QInputDialog::getText(this, title, prompt, QLineEdit::Password, {}, &accepted);
        if (!accepted || password.isEmpty() || !authenticateAccount(account, password)) {
            account->clearSession(account->getRememberSession() && !needsProxyPassword);
            return;
        }
        saveSettings();
    }

    const bool refreshed = refreshGameAccounts(account);
    if (needsProxyPassword && !signingIn) {
        if (refreshed && account->getRememberSession())
            saveSettings();
        else if (!refreshed && account->hasSession())
            account->setProxyPassword({});
    }
    updateSessionControls();
}

void MainWindow::on_signOutButton_clicked()
{
    auto *account = selectedGameforgeAccount();
    if (accountOperationInProgress || !account)
        return;

    const QScopedValueRollback accountOperation(accountOperationInProgress, true);
    clearAccountSession(account);
    saveSettings();
}

void MainWindow::on_rememberSessionCheckBox_toggled(bool checked)
{
    auto *account = selectedGameforgeAccount();
    if (accountOperationInProgress || !account) {
        updateSessionControls();
        return;
    }

    const QScopedValueRollback accountOperation(accountOperationInProgress, true);
    account->setRememberSession(checked);
    saveSettings();
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
    if (accountOperationInProgress || ui->accountsListWidget->currentRow() < 0)
        return;

    if (!checkGameClientPath())
        return;

    const auto &profileAccounts = profiles.at(ui->profileComboBox->currentIndex())->getAccounts();
    QQueue<GameAccount> accounts;
    for (auto *item : ui->accountsListWidget->selectedItems()) {
        const int row = ui->accountsListWidget->row(item);
        accounts.enqueue(profileAccounts.at(row));
    }

    openAccount(accounts);
}

GameforgeAccount *MainWindow::selectedGameforgeAccount() const
{
    const int index = ui->gameforgeAccountComboBox->currentIndex();
    return index >= 0 && index < gfAccounts.size() ? gfAccounts.at(index) : nullptr;
}

void MainWindow::updateSessionControls()
{
    const auto *account = selectedGameforgeAccount();
    const bool hasSession = account && account->hasSession();
    const QSignalBlocker blocker(ui->rememberSessionCheckBox);
    ui->rememberSessionCheckBox->setEnabled(account != nullptr);
    ui->rememberSessionCheckBox->setChecked(account && account->getRememberSession());
    ui->signInButton->setEnabled(account != nullptr);
    ui->signOutButton->setEnabled(hasSession);

    QString action = tr("Sign in");
    if (hasSession)
        action = account->needsProxyPassword() ? tr("Connect") : tr("Refresh");
    ui->signInButton->setText(action);
}

void MainWindow::on_gameforgeAccountComboBox_currentIndexChanged(int index)
{
    updateSessionControls();
    if (index < 0)
        return;

    QString tooltipText;

    GameforgeAccount* gf = gfAccounts.at(index);

    if (gf->getAuth()->getUseProxy()) {
        tooltipText += "Proxy IP: " + gf->getAuth()->getProxyIp() + "\n";
        tooltipText += "Proxy port: " + gf->getAuth()->getSocksPort();
    }
    else {
        tooltipText += "No proxy";
    }

    if (gf->getcustomClientPath().isEmpty()) {
        tooltipText += "\nUsing default nostale client";
    }
    else {
        tooltipText += "\nCustom client: " + gf->getcustomClientPath().right(gf->getcustomClientPath().size() - gf->getcustomClientPath().lastIndexOf("/") - 1);
    }

    ui->gameforgeAccountComboBox->setToolTip(tooltipText);
}


void MainWindow::on_createGameAccButton_clicked()
{
    if (accountOperationInProgress)
        return;

    const QScopedValueRollback accountOperation(accountOperationInProgress, true);
    QVector<GameforgeAccount*> signedInAccounts;
    for (auto *account : gfAccounts) {
        if (account->hasSession() && !account->needsProxyPassword())
            signedInAccounts.push_back(account);
    }
    if (signedInAccounts.isEmpty()) {
        ui->statusbar->showMessage(tr("Sign in to a Gameforge account first."), 10000);
        return;
    }

    CreateGameAccountDialog dialog(signedInAccounts, settingsDialog->getGameLanguage(), this);
    if (dialog.exec() == QDialog::Accepted) {
        QJsonValue accInfo = dialog.getNewAccountInfo();

        QString displayName = accInfo["displayName"].toString();
        QString id = accInfo["accountId"].toString();

        GameAccount gameAcc(dialog.getGfAcc(), displayName, id, displayName, defaultServerLocation, defaultServer,
                            defaultChannel, defaultCharacter, defaultAutoLogin);
        profiles.first()->addAccount(gameAcc);
        profiles.first()->sort();

        displayProfile(ui->profileComboBox->currentIndex());
    }
    for (auto *account : signedInAccounts) {
        if (!account->hasSession())
            sessionExpired(account);
    }
}


void MainWindow::on_repairButton_clicked()
{
    updateGame();
}

void MainWindow::openAccount(QQueue<GameAccount> accounts)
{
    if (accounts.isEmpty())
        return;

    if (accountOperationInProgress) {
        ui->statusbar->showMessage(tr("Another account operation is in progress. Try again."), 10000);
        return;
    }

    const QScopedValueRollback accountOperation(accountOperationInProgress, true);
    launchAccount(accounts.dequeue());
    if (!accounts.isEmpty()) {
        const int delay = settingsDialog->getOpenInterval() * 1000;
        QTimer::singleShot(delay, this, [this, accounts] { openAccount(accounts); });
    }
}

void MainWindow::launchAccount(const GameAccount &gameAccount)
{
    const qsizetype index = gfAccounts.indexOf(gameAccount.getGfAcc());
    if (index < 0)
        return;

    auto *account = gfAccounts.at(index);
    if (!account->hasSession() || account->needsProxyPassword()) {
        ui->statusbar->showMessage(tr("Sign in or connect as %1 first.").arg(account->getEmail()), 10000);
        return;
    }

    ui->statusbar->showMessage(tr("Trying to open account %1").arg(gameAccount.getName()), 10000);
    const QString code = account->getToken(gameAccount.getId());
    if (code.isEmpty()) {
        if (!account->hasSession())
            sessionExpired(account);
        else
            ui->statusbar->showMessage(tr("Couldn't get a launch code for %1.").arg(gameAccount.getName()), 10000);
        return;
    }

    DWORD pid = 0;
    QString path = account->getcustomClientPath();
    if (path.isEmpty())
        path = settingsDialog->getGameClientPath();
    const int language = settingsDialog->getGameLanguage();
    if (gflessClient->openClient(gameAccount.getName(), code, path, language, pid)) {
        processAccounts.insert(pid, gameAccount);
        ui->statusbar->showMessage(tr("Launched game with PID %1").arg(pid), 10000);
    }
    else {
        ui->statusbar->showMessage(tr("Failed to launch game for %1.").arg(gameAccount.getName()), 10000);
    }
}
