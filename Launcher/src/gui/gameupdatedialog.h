#ifndef GAMEUPDATEDIALOG_H
#define GAMEUPDATEDIALOG_H

#include <QDialog>
#include "gameupdater.h"
#include <QThread>

namespace Ui {
class GameUpdateDialog;
}

class GameUpdateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GameUpdateDialog(GameUpdater* gameUpdater, QWidget *parent = nullptr);

    ~GameUpdateDialog();

private:
    Ui::GameUpdateDialog *ui;
    GameUpdater* updater;
    QThread* updateThread;
};

#endif // GAMEUPDATEDIALOG_H
