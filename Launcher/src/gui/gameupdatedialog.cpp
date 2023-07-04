#include "gameupdatedialog.h"
#include "ui_gameupdatedialog.h"
#include <QTimer>


GameUpdateDialog::GameUpdateDialog(GameUpdater *gameUpdater, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GameUpdateDialog),
    updater(gameUpdater)
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setWindowFlag(Qt::WindowCloseButtonHint, false);

    connect(updater, &GameUpdater::setProgress, ui->updateProgressBar, &QProgressBar::setValue);

    updateThread = QThread::create(&GameUpdater::updateFiles, updater);
    updateThread->start();

    connect(updateThread, &QThread::finished, this, &GameUpdateDialog::accept);
}

GameUpdateDialog::~GameUpdateDialog()
{
    delete updateThread;
    delete ui;
}
