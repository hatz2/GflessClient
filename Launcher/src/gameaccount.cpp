#include "gameaccount.h"
#include "gameforgeaccount.h"

GameAccount::GameAccount(GameforgeAccount *gameforgeAcc, const QString &gameAccName, const QString &gameAccId)
    : GameAccount(gameforgeAcc, gameAccName, gameAccId, gameAccName, 0, 0, 0, 0, false)
{

}

GameAccount::GameAccount(GameforgeAccount *gameforgeAcc, const QString &gameAccName, const QString &gameAccId, const QString &fakeName, int serverLoc, int serverIndex, int channelIndex, int slotIndex, bool login)
    : gfAcc(gameforgeAcc)
    , name(gameAccName)
    , id(gameAccId)
    , displayName(fakeName)
    , serverLocation(serverLoc)
    , server(serverIndex)
    , channel(channelIndex)
    , slot(slotIndex)
    , autoLogin(login)
{

}

GameAccount::GameAccount()
    : GameAccount(nullptr, QString(), QString())
{

}

QString GameAccount::getId() const
{
    return id;
}

QString GameAccount::getDisplayName() const
{
    return displayName;
}

QString GameAccount::getName() const
{
    return name;
}

const GameforgeAccount *GameAccount::getGfAcc() const
{
    return gfAcc;
}

int GameAccount::getServerLocation() const
{
    return serverLocation;
}

int GameAccount::getServer() const
{
    return server;
}

int GameAccount::getChannel() const
{
    return channel;
}

int GameAccount::getSlot() const
{
    return slot;
}

bool GameAccount::getAutoLogin() const
{
    return autoLogin;
}

void GameAccount::setServerLocation(int newServerLocation)
{
    serverLocation = newServerLocation;
}

void GameAccount::setServer(int newServer)
{
    server = newServer;
}

void GameAccount::setChannel(int newChannel)
{
    channel = newChannel;
}

void GameAccount::setSlot(int newSlot)
{
    slot = newSlot;
}

void GameAccount::setAutoLogin(bool newAutoLogin)
{
    autoLogin = newAutoLogin;
}

QString GameAccount::toString() const
{
    QString output;

    if (autoLogin) {
        output += "Server: " + QString::number(server + 1);
        output += " | Ch: " + QString::number(channel + 1);
        output += " | Char: ";

        if (slot == 0) {
            output += "Login screen";
        }
        else {
            output += QString::number(slot);
        }

        output += " | ";
    }

    output += displayName;

    return output;
}
