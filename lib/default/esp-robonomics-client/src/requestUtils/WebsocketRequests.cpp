#include "config.h"

#ifdef ROBONOMICS_USE_WS

#include "WebsocketRequests.h"

void WebsocketRequests::setup(String host) {
    wsUtils.setupWebsocket(host);
}

void WebsocketRequests::disconnect() {
    wsUtils.disconnect();
}

JSONVar WebsocketRequests::sendRequest(String message) {
    wsUtils.setOnTextCallback([this](uint8_t *payload) {resultCallback(payload);});
    wsUtils.sendMessage(message);
    while (!got_response) {
        wsUtils.websocketLoop();
    }
    got_response = false;
    return response;
}

void WebsocketRequests::resultCallback(uint8_t *payload) {
    // ESP_LOGI(TAG, "Extrinsic result: %s", (char *)payload);
    response = JSON.parse((char *)payload);
    got_response = true;
}

#endif