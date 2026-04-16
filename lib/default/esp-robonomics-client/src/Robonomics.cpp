#include "Robonomics.h"
#include <inttypes.h>
#include "address.h"
#include <Arduino.h>
#include <Ed25519.h>

static const char *TAG = "ROBONOMICS";

void Robonomics::setup(String host) {
    blockchainUtils.setup(host);
}

void Robonomics::disconnectWebsocket() {
    blockchainUtils.disconnect();
}

void Robonomics::generateAndSetPrivateKey() {
    uint8_t robonomicsPrivateKey[32];
    Ed25519::generatePrivateKey(robonomicsPrivateKey);
    setPrivateKey(robonomicsPrivateKey);
}

void Robonomics::setPrivateKey(const char* hexPrivateKey) {
    uint8_t privateKey[KEYS_SIZE];
    hex2bytes(hexPrivateKey, privateKey);
    setPrivateKey(privateKey);
}

void Robonomics::setPrivateKey(uint8_t *privateKey) {
    memcpy(privateKey_, privateKey, KEYS_SIZE);
    Ed25519::derivePublicKey(publicKey_, privateKey_);
    char* tempAddress = getAddrFromPrivateKey(privateKey_, ROBONOMICS_PREFIX);
    if (tempAddress != nullptr) {
        strncpy(ss58Address, tempAddress, SS58_ADDRESS_SIZE); // Leave space for null terminator
        ss58Address[SS58_ADDRESS_SIZE] = '\0'; // Ensure null termination
        delete[] tempAddress;
        Serial.printf("Robonomics Address: %s\r\n", ss58Address);
    } else {
        Serial.println("Failed to get address from public key.");
    }
}

const char* Robonomics::getPrivateKey() const {
    static char hexString[KEYS_SIZE * 2 + 1];
    size_t length = sizeof(privateKey_) / sizeof(privateKey_[0]);
    bytes2hex(privateKey_, length, hexString);
    return hexString;
}

const char* Robonomics::getSs58Address() const {
    return ss58Address;
}

const char* Robonomics::sendCustomCall() {
    Data call = createCall();

    const char* res = createAndSendExtrinsic(call);
    return res;
}

const char* Robonomics::sendDatalogRecord(const std::string& data) {
    Data head_dr_  = Data{0x33,0};
    Data call = callDatalogRecord(head_dr_, data);

    const char* res = createAndSendExtrinsic(call);
    return res;
}

const char* Robonomics::sendRWSDatalogRecord(const std::string& data, const char *owner_address) {
    Data head_dr_ = Data{0x33,0};
    Data head_rws_ = Data{0x37,0};
    Data call_nested = callDatalogRecord(head_dr_, data);
    RobonomicsPublicKey ownerKey = getPublicKeyFromAddr(owner_address);
    Data call = callRws(head_rws_, ownerKey, call_nested);

    const char* res = createAndSendExtrinsic(call);
    return res;
}

const char* Robonomics::createAndSendExtrinsic(Data call) {
    const char* error_res = "error";

    uint64_t payloadNonce;
    if (!getNonce(& blockchainUtils, ss58Address, &payloadNonce)) return error_res;
    std::string payloadBlockHash;
    if (!getGenesisBlockHash(& blockchainUtils, &payloadBlockHash)) return error_res;
    payloadBlockHash.erase(0, 2);
    uint32_t payloadEra = getEra();
    uint64_t payloadTip = getTip();
    uint32_t payloadSpecVersion;
    uint32_t payloadTransactionVersion;
    if (!extractRuntimeVersions(& blockchainUtils, & payloadSpecVersion, & payloadTransactionVersion)) return error_res;
    // Serial.printf("Spec version: %" PRIu32 ", tx version: %" PRIu32 ", nonce: %llu, era: %" PRIu32 ", tip: %llu\r\n", payloadSpecVersion, payloadTransactionVersion, (unsigned long long)payloadNonce, payloadEra, (unsigned long long)payloadTip);
    Data data_ = createPayload(call, payloadEra, payloadNonce, payloadTip, payloadSpecVersion, payloadTransactionVersion, payloadBlockHash, payloadBlockHash);
    Data signature_ = createSignature(data_, privateKey_, publicKey_);
    std::vector<std::uint8_t> pubKey( reinterpret_cast<std::uint8_t*>(std::begin(publicKey_)), reinterpret_cast<std::uint8_t*>(std::end(publicKey_)));
    Data edata_ = createSignedExtrinsic(signature_, pubKey, payloadEra, payloadNonce, payloadTip, call);
    int requestId = blockchainUtils.getRequestId();
    const char* res = sendExtrinsic(edata_, requestId);
    return res;
}

void Robonomics::signMessage(const String &message, String &signature) {
    std::string msgStr = std::string(message.c_str());
    // std::string msgStr(message.c_str());
    Data msgData(msgStr.begin(), msgStr.end());
    Data sigData = createSignature(msgData, privateKey_, publicKey_);
    for (int k = 0; k < sigData.size(); k++) 
        printf("%02x", sigData[k]);
    printf("\r\n");
    String sigHex = "";
    for (size_t i = 0; i < sigData.size(); i++) {
        if (sigData[i] < 0x10) {
            sigHex += "0"; // pad with a zero if needed
        }
        sigHex += String(sigData[i], HEX);
    }
    signature = sigHex;
}

Data Robonomics::createCall() {
    Data call;
    std::vector<uint8_t> callStr = hex2bytes(CALL_ENCODED);
    append(call, callStr);
    Serial.printf("Call size: %zu\r\n", call.size());
    // for (int k = 0; k < call.size(); k++) 
    //     printf("%02x", call[k]);
    // printf("\r\n");
    return call;
}

Data Robonomics::createPayload(Data call, uint32_t era, uint64_t nonce, uint64_t tip, uint32_t sv, uint32_t tv, std::string gen, std::string block) {
    Data data_ = doPayload (call, era, nonce, tip, sv, tv, gen, block);
    Serial.printf("Payload size: %zu\r\n", data_.size());
    // for (int k = 0; k < data_.size(); k++) 
    //     printf("%02x", data_[k]);
    // printf("\r\n");
    return data_;
}

Data Robonomics::createSignature(Data data, uint8_t privateKey[32], uint8_t publicKey[32]) {
    Data signature_ = doSign (data, privateKey, publicKey);
    Serial.printf("Signature size: %zu\r\n", signature_.size());
    // for (int k = 0; k < signature_.size(); k++) 
    //     printf("%02x", signature_[k]);
    // printf("\r\n");
    return signature_;
}

Data Robonomics::createSignedExtrinsic(Data signature, Data pubKey, uint32_t era, uint64_t nonce, uint64_t tip, Data call) {
    Data edata_ = doEncode (signature, pubKey, era, nonce, tip, call);
    Serial.printf("Extrinsic %s: size %zu\r\n", "Datalog", edata_.size());
    // for (int k = 0; k < edata_.size(); k++) 
    //     printf("%02x", edata_[k]);
    // printf("\r\n");
    return edata_;
}

const char* Robonomics::sendExtrinsic(Data extrinsicData, int requestId) {
    String extrinsicMessage = fillParamsJs(extrinsicData, requestId);
    Serial.printf("After to string: %s\r\n", extrinsicMessage.c_str());
    // Serial.print(extrinsicMessage);
    JSONVar result = blockchainUtils.rpcRequest(extrinsicMessage);
    String extrinsicResult;
    if (result.hasOwnProperty("result")) {
        extrinsicResult = JSON.stringify(result["result"]);
    } else {
        extrinsicResult = JSON.stringify(result["error"]);
    }
    Serial.printf("Extrinsic result: %s", extrinsicResult.c_str());
    return extrinsicResult.c_str();
}
