#ifndef __requesthandler_h__
#define __requesthandler_h__

#include <QString>
#include <QByteArray>

// Aliases for common using in global namespace

// enum ConnectionState
#define InitialState        (RequestHandler::Initial)
#define EstablishState      (RequestHandler::Establish)
#define ProgressState       (RequestHandler::Progress)
#define CloseWaitState      (RequestHandler::Closing)
#define UnconnectState      (RequestHandler::Unconnect)
#define ErrorState          (RequestHandler::HasError)
#define WarningState        (RequestHandler::HasWarning)

// enum DisconnectStatus
#define NoDisconnect        (RequestHandler::None)
#define ClientDisconnect    (RequestHandler::Client)
#define RemoteDisconnect    (RequestHandler::Remote)
#define ForcedDisconnect    (RequestHandler::Forced)


///////////////////////////
class RequestHandler
{
public:
    enum ConnectionState {
        Initial     = -1,
        Establish   = Initial + 1,
        Progress    = Initial + 2,
        Closing     = Initial + 3,
        Unconnect   = Initial + 4,
        HasError    = Initial + 5,
        HasWarning  = Initial + 6,
    };

    enum DisconnectStatus {
        None    = Establish,
        Client  = 8,
        Remote  = 16,
        Forced  = 32,
    };

    virtual ~RequestHandler() {};

    virtual void onStateChanged(int state, short disconnectStatus) = 0;
    virtual void onMessageReceived(const QByteArray& message) = 0;
    virtual void onHaveToLogin() = 0;
    virtual void onHaveToLogout() = 0;
    virtual void onHaveToTestRequest() = 0;
    virtual void onHaveToHeartbeat() = 0;
    virtual bool onHaveToSendMessage(const QByteArray& message) = 0;

    virtual QString errorString() const = 0;
    virtual short disconnectStatus() const = 0;
};

#endif /* __requesthandler_h__ */
