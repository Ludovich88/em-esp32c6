#include "config.h"

#ifdef ROBONOMICS_USE_HTTP

#include <Arduino.h>
#include "HTTPRequests.h"
#ifdef ESP32
#include <HTTPClient.h>
#endif
#ifdef ESP8266
#include <ESP8266HTTPClient.h>
#endif


void HTTPRequests::setup(String host) {
    node_url = "http://" + host + "/rpc/";
}

void HTTPRequests::disconnect() {}

JSONVar HTTPRequests::sendRequest(String message) {
    Serial.print("[HTTP]+POST:\n"); 
    JSONVar response;
    HTTPClient http;
    http.begin(wifi_client, node_url.c_str());
    http.addHeader("Content-Type", "application/json");
    uint32_t httpCode = (uint32_t)http.POST(message);
    Serial.println("sent:");
    Serial.println(message);
    if (httpCode > 0) {
        Serial.printf("[HTTP]+POST code: %d\n", httpCode);
        if (httpCode == HTTP_CODE_OK) {
            const String& payload = http.getString();
            Serial.println("received:");
            Serial.println(payload);
            response = JSON.parse(payload);
        } else {
            Serial.println("HTTP response is not 200");
            response["error"] = httpCode;
        }
    } else {
        Serial.println("HTTP response is 0");
        response["error"] = httpCode;
    }
    return response;
}

#endif