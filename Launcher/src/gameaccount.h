#ifndef GAMEACCOUNT_H
#define GAMEACCOUNT_H

#include <QString>

class GameAccount
{
public:
    explicit GameAccount(class GameforgeAccount* gameforgeAcc, const QString& gameAccName, const QString& gameAccId);

    explicit GameAccount(class GameforgeAccount* gameforgeAcc, const QString& gameAccName, const QString& gameAccId,
                         const QString& fakeName, int serverLoc, int serverIndex, int channelIndex, int slotIndex,
                         bool login);

    explicit GameAccount();


    QString getId() const;

    QString getDisplayName() const;

    QString getName() const;

    const GameforgeAccount *getGfAcc() const;

    int getServerLocation() const;

    int getServer() const;
    int getChannel() const;
    int getSlot() const;
    bool getAutoLogin() const;

    void setServerLocation(int newServerLocation);
    void setServer(int newServer);
    void setChannel(int newChannel);
    void setSlot(int newSlot);
    void setAutoLogin(bool newAutoLogin);

    QString toString() const;

private:
    class GameforgeAccount* gfAcc;
    QString name;
    QString id;
    QString displayName;
    int serverLocation;
    int server;
    int channel;
    int slot;
    bool autoLogin;
};

#endif // GAMEACCOUNT_H
