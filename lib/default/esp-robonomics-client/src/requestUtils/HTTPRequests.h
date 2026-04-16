#pragma once

#include "config.h"

#ifdef ROBONOMICS_USE_HTTP
#ifdef ESP32
#include <WiFi.h>
#endif
#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif
#include <Arduino_JSON.h>

class HTTPRequests {
    private:
        WiFiClient wifi_client;
        String node_url;
    public:
        void setup(String host);
        void disconnect();
        JSONVar sendRequest(String message);
};

#endif