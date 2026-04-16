#include <Arduino.h>
#include <Ed25519.h>
#include <address.h>
#include <Robonomics.h>
#include <WiFi.h>

#define USE_RWS
#define RWS_OWNER "4CJDTkd81vK4Wp2BxiQUVJmhYptAQoiTKUTH4JzeAECYnv99"

#define WIFI_SSID "WifiSSID"
#define WIFI_PASSWORD "WifiPassword"
#define SENDING_DELAY 20000

uint8_t counter = 0;
String robonomics_host = "polkadot.rpc.robonomics.network";
uint16_t last_send_time = 0

Robonomics robonomics;

void setup() {
    Serial.begin(115200);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while ( WiFi.status() != WL_CONNECTED ) {
        vTaskDelay(500 /portTICK_PERIOD_MS);
        Serial.print("." );
    }
    robonomics.generateAndSetPrivateKey();
    robonomics.setup(robonomics_host);
    Serial.printf("Robonomics address: %s\r\n", robonomics.getSs58Address());
    Serial.printf("Robonomics private key: %s\r\n", robonomics.getPrivateKey());
}

void loop() {
    if (millis() - last_send_time > SENDING_DELAY) {
        const char* res;
        String data = "Counter: " + String(counter);
#ifdef USE_RWS
        res = robonomics.sendRWSDatalogRecord(data, RWS_OWNER);
#else
        res = robonomics.sendDatalogRecord(data);
#endif
        Serial.printf("Extrinsix result: %s", res);
        counter++;
        last_send_time = millis();
    }
}