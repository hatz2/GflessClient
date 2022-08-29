#ifndef CAPTCHADIALOG_H
#define CAPTCHADIALOG_H

#include "captchasolver.h"
#include <QDialog>
#include <QPixmap>
#include <QMessageBox>
#include <QLocale>

namespace Ui {
class CaptchaDialog;
}

class CaptchaDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CaptchaDialog(const QString& gfChallengeId, QWidget *parent = nullptr);
    ~CaptchaDialog();

private slots:
    void on_sendAnswerBton_clicked();

private:
    Ui::CaptchaDialog *ui;
    CaptchaSolver* captcha;

    void initImages();
};

#endif // CAPTCHADIALOG_H
