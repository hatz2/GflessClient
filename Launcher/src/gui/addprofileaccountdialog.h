#ifndef ADDPROFILEACCOUNTDIALOG_H
#define ADDPROFILEACCOUNTDIALOG_H

#include <QDialog>
#include <QMessageBox>
#include <QVector>
#include "gameforgeaccount.h"
#include "gameaccount.h"

namespace Ui {
class AddProfileAccountDialog;
}

class AddProfileAccountDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddProfileAccountDialog(const QVector<GameforgeAccount*> accounts, QWidget *parent = nullptr);

    explicit AddProfileAccountDialog(const QVector<GameforgeAccount*> accounts,
                                     const GameAccount& gameAccount,
                                     QWidget *parent = nullptr);
    ~AddProfileAccountDialog();

    const QString &getAccountName() const;

    const QString &getPseudonym() const;

    bool getAutoLogin() const;

    int getServer() const;

    int getChannel() const;

    int getCharacter() const;

    GameforgeAccount *getGfAcc() const;

    QString getId() const;

    int getServerLocation() const;

private slots:
    void on_addProfAccountButton_clicked();

    void on_loginCheckBox_stateChanged(int arg1);

    void on_gameforgeAccountComboBox_currentIndexChanged(int index);

private:
    static bool autoLoginDefault;
    static int serverLocationDefault;
    static int serverDefault;
    static int channelDefault;
    static int characterDefault;


    Ui::AddProfileAccountDialog *ui;
    QVector<GameforgeAccount*> gfAccounts;
    GameforgeAccount* gfAcc;
    QString accountName;
    QString pseudonym;
    QString id;
    bool autoLogin;
    int serverLocation;
    int server;
    int channel;
    int character;
};

#endif // ADDPROFILEACCOUNTDIALOG_H
