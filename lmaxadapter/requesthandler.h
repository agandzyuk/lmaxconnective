#ifndef __requesthandler_h__
#define __requesthandler_h__

#include <QString>
#include <QByteArray>

// Aliases for common using in global namespace

// enum ConnectionState
#define InitialState        (RequestHandler::Initial)
#define EstablishState      (RequestHandler::Establish)
#define EstablishWarnState  (RequestHandler::EstablishWithWarning)
#define ProgressState       (RequestHandler::Connecting)
#define ReconnectingState   (RequestHandler::Reconnecting)
#define EngineClosingState  (RequestHandler::EngineClosing)
#define ForcedClosingState  (RequestHandler::ForcedClosing)
#define ClosedRemoteState   (RequestHandler::DisconnectByRemote)
#define ClosedFailureState  (RequestHandler::DisconnectByFailure)
#define UnconnectState      (RequestHandler::Unconnect)


///////////////////////////
class RequestHandler
{
public:
    enum ConnectionState {
        Initial = 0,
        Establish,
        EstablishWithWarning,
        Connecting,
        Reconnecting,
        EngineClosing,
        ForcedClosing,
        DisconnectByRemote,
        DisconnectByFailure,
        Unconnect,
    };

    virtual ~RequestHandler() {};

    virtual void onStateChanged(RequestHandler::ConnectionState state) = 0;
    virtual void onMessageReceived(const QByteArray& message) = 0;
    virtual void onHaveToLogin() = 0;
    virtual void onHaveToLogout() = 0;
    virtual void onHaveToTestRequest() = 0;
    virtual void onHaveToHeartbeat() = 0;
    virtual bool onHaveToSendMessage(const QByteArray& message) = 0;
};

#endif /* __requesthandler_h__ */
