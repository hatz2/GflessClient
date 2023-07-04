#ifndef CREATEGAMEACCOUNTDIALOG_H
#define CREATEGAMEACCOUNTDIALOG_H

#include <QDialog>
#include <QVector>
#include "gameforgeaccount.h"


namespace Ui {
class CreateGameAccountDialog;
}

class CreateGameAccountDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateGameAccountDialog(const QVector<GameforgeAccount*>& gfAccs, const int gfLangIndex, QWidget *parent = nullptr);
    ~CreateGameAccountDialog();

    QJsonObject getNewAccountInfo() const;

    GameforgeAccount *getGfAcc() const;

private slots:
    void on_createAccButton_clicked();

private:
    Ui::CreateGameAccountDialog *ui;
    QVector<GameforgeAccount*> gfAccounts;
    GameforgeAccount* gfAcc;
    QJsonObject newAccountInfo;
    QString gfLang;
};

#endif // CREATEGAMEACCOUNTDIALOG_H
