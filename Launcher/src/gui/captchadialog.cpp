#include "captchadialog.h"
#include "ui_captchadialog.h"

CaptchaDialog::CaptchaDialog(const QString &gfChallengeId, SyncNetworAccesskManager *netManager, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CaptchaDialog)
{
    ui->setupUi(this);
    captcha = new CaptchaSolver(gfChallengeId, QLocale().name().replace("_", "-"), netManager, this);

    initImages();
}

CaptchaDialog::~CaptchaDialog()
{
    delete captcha;
    delete ui;
}

void CaptchaDialog::initImages()
{
    captcha->getChallenge();
    ui->textLabel->setPixmap(QPixmap::fromImage(captcha->getTextImage()));
    ui->dragIconsLabel->setPixmap(QPixmap::fromImage(captcha->getDragIcons()));
    ui->dropTargetLabel->setPixmap(QPixmap::fromImage(captcha->getDropTargetImage()));
}

void CaptchaDialog::on_sendAnswerBton_clicked()
{
    int answer = ui->answerSpinbox->value() - 1;

    if (captcha->sendAnswer(answer))
    {
        QMessageBox::information(this, "Success", "Captcha solved successfully");
        accept();
    }
    else
    {
        QMessageBox::critical(this, "Error", "Incorrect answer");
        initImages();
    }
}

