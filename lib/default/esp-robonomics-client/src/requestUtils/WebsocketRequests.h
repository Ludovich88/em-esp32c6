#pragma once

#include "config.h"

#ifdef ROBONOMICS_USE_WS

#include "WebsocketUtils.h"
#include <Arduino_JSON.h>

class WebsocketRequests {
    private:
        JSONVar response;
        bool got_response;
        WebsocketUtilsRobonomics wsUtils;
        void resultCallback(uint8_t *payload);
    public:
        void setup(String host);
        void disconnect();
        JSONVar sendRequest(String message);
};

#endif