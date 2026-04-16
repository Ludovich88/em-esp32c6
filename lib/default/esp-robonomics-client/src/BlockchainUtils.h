#pragma once

#include <Arduino_JSON.h>
#include <Arduino.h>
#include "requestUtils/config.h"

#ifdef ROBONOMICS_USE_HTTP
#include "requestUtils/HTTPRequests.h"
#endif

#ifdef ROBONOMICS_USE_WS
#include "requestUtils/WebsocketUtils.h"
#endif

class BlockchainUtils {
private:
    int requestId = 1;
#ifdef ROBONOMICS_USE_HTTP
    HTTPRequests requestUtils;
#elif ROBONOMICS_USE_WS
    WebsocketRequests requestUtils;
#endif
public:
    void setup(String host);
    void disconnect();
    int getRequestId();
    JSONVar rpcRequest(String data);
    String createWebsocketMessage(String method, JSONVar paramsArray);
};
