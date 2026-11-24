#ifndef CLIENTTITLETAGGER_H
#define CLIENTTITLETAGGER_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <Windows.h>

/*
 * Appends " [<tag>]" (the egress IP of the account) to the main window title
 * of a launched game client, identified by its process id. The game sets its
 * own caption, so the suffix is re-applied on a timer; the tagger removes
 * itself when the game process exits.
 */
class ClientTitleTagger : public QObject
{
    Q_OBJECT
public:
    ClientTitleTagger(QObject* parent, DWORD pid, const QString& tag);
    ~ClientTitleTagger() override;

private slots:
    void poll();

private:
    struct EnumContext
    {
        DWORD pid;
        HWND best;
        LONG_PTR bestArea;
    };

    static BOOL CALLBACK enumProc(HWND hwnd, LPARAM lParam);
    HWND findGameWindow() const;

    DWORD pid;
    QString tag;
    QString appliedSuffix;
    QTimer* timer;
    HANDLE process;
};

#endif // CLIENTTITLETAGGER_H
