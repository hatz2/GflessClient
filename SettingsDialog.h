#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QFileDialog>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    QString getGameClientPath() const;
    int getOpenInterval() const;
    int getGameLanguage() const;

    void setGameClientPath(const QString& path);
    void setOpenInterval(int n);
    void setGameLanguage(int language);

private slots:
    void on_selectGamePathButton_clicked();

private:
    Ui::SettingsDialog *ui;
};


#endif // SETTINGSDIALOG_H
