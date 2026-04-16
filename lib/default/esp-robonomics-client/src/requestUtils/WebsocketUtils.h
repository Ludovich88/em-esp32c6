#pragma once

#include "config.h"

#ifdef ROBONOMICS_USE_WS

#include <WebSocketsClient.h>
#include "Defines.h"

typedef void (*WebSocketClientEvent)(WStype_t type, uint8_t * payload, size_t length);
typedef std::function<void(uint8_t *payload)> OnTextWebsocketCallback;

class WebsocketUtilsRobonomics {
private:
    OnTextWebsocketCallback onTextCallback;
    WebSocketsClient webSocket;
    bool websocketConnected = false;
    bool connectionClosed = true;
public:
    void websocketLoop();
    void disconnect();
    void sendMessage(String text);
    void setupWebsocket(String host);
    void disconnectWebSocket();
    void connectWebscoket();
    void setOnTextCallback(OnTextWebsocketCallback callback);
    void mainWebsocketCallback(WStype_t type, uint8_t *payload, size_t length);
};

void defaultOnTextCallback(uint8_t *payload);


#endif