#include "BlockchainUtils.h"

void BlockchainUtils::setup(String host) {
    requestUtils.setup(host);
}

void BlockchainUtils::disconnect() {
    requestUtils.disconnect();
}

JSONVar BlockchainUtils::rpcRequest(String data) {
    JSONVar res = requestUtils.sendRequest(data);
    requestId++;
    return res;
}

String BlockchainUtils::createWebsocketMessage(String method, JSONVar paramsArray) {
    JSONVar messageObject;
    messageObject["jsonrpc"] = "2.0";
    messageObject["method"] = method;
    messageObject["params"] = paramsArray;
    messageObject["id"] = requestId;
    String messageString = JSON.stringify(messageObject);
    return messageString;
}

int BlockchainUtils::getRequestId() {
    return requestId;
}