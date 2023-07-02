#ifndef EDITMULTIPLEPROFILEACCOUNTSDIALOG_H
#define EDITMULTIPLEPROFILEACCOUNTSDIALOG_H

#include <QDialog>

namespace Ui {
class EditMultipleProfileAccountsDialog;
}

class EditMultipleProfileAccountsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditMultipleProfileAccountsDialog(QWidget *parent = nullptr);
    ~EditMultipleProfileAccountsDialog();

    bool getAutoLogin() const;
    int getServerLocation() const;
    int getServer() const;
    int getChannel() const;
    int getCharacter() const;

private slots:
    void on_loginCheckBox_stateChanged(int arg1);

    void on_addProfAccountButton_clicked();

private:
    Ui::EditMultipleProfileAccountsDialog *ui;

    static bool defaultAutoLogin;
    static int defaultServerLocation;
    static int defaultServer;
    static int defaultChannel;
    static int defaultCharacter;

    bool autoLogin;
    int serverLocation;
    int server;
    int channel;
    int character;
};

#endif // EDITMULTIPLEPROFILEACCOUNTSDIALOG_H
