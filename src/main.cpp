#include <Ethernet.h>
#include <ArduinoHA.h>
#include <WiFiNINA.h>
#include <utility/wifi_drv.h>
#include <ArduinoLog.h>

#include "arduino_secrets.h"
#include "constants.h"
#include "momentary_relay.h"

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

int status = WL_IDLE_STATUS;

const char *sensorNames[]{nullptr, A_1, A_2, A_3, A_4};
boolean sensorStates[] = {LOW, LOW, LOW, LOW, LOW};
unsigned long lastReadAt = millis();

int relay[] = {0, 1, 2};
byte mac[] = {0x24, 0x0a, 0xc4, 0xc3, 0xc5, 0x24};

WiFiClient client;

HADevice device(mac, sizeof(mac));
HAMqtt mqtt(client, device, 20);

HASwitch relay1(RELAY_1);
HASwitch mrelay1(MOMENTARY_RELAY_1);

MomentaryRelay momentaryRelay1(&mrelay1, &relay1, relay[1]);

HASwitch relay2(RELAY_2);
HASwitch mrelay2(MOMENTARY_RELAY_2);

MomentaryRelay momentaryRelay2(&mrelay2, &relay2, relay[2]);

HABinarySensor *a1 = new HABinarySensor(sensorNames[1]);
HABinarySensor *a2 = new HABinarySensor(sensorNames[2]);
HABinarySensor *a3 = new HABinarySensor(sensorNames[3]);
HABinarySensor *a4 = new HABinarySensor(sensorNames[4]);
HABinarySensor *sensors[] = {nullptr, a1, a2, a3, a4};

void onSwitchCommand(bool state, HASwitch *sender) {
    if (sender == &relay1) {
        digitalWrite(relay[1], state ? HIGH : LOW);
    }
    if (sender == &mrelay1) {
        momentaryRelay1.activate();
    }
    if (sender == &relay2) {
        digitalWrite(relay[2], state ? HIGH : LOW);
    }
    if (sender == &mrelay2) {
        momentaryRelay2.activate();
    }

    sender->setState(state); // report state back to the Home Assistant
}

void showColor(int red, int green, int blue) {
    WiFiDrv::analogWrite(RED, red);
    WiFiDrv::analogWrite(GREEN, green);
    WiFiDrv::analogWrite(BLUE, blue);
}

void printWiFiStatus() {
    // print the SSID of the network you're attached to:
    Log.notice(F("SSID: %s" CR), WiFi.SSID());

    // print your board's IP address:
    IPAddress ip = WiFi.localIP();
    Log.notice(F("IP Address: %p" CR), ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Log.notice(F("signal strength (RSSI): %l dBm" CR), rssi);
    showColor(100, 0, 100);
}

void connectToWiFi() {
    status = WL_IDLE_STATUS;
    while (status != WL_CONNECTED) {
        Log.notice(F("Attempting to connect to SSID: %s" CR), ssid);

        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        status = WiFi.begin(ssid, pass);

        // wait 10 seconds for connection:
        showColor(10, 5, 5);

        if (status != WL_CONNECTED) {
            delay(10000);
        }

        showColor(10, 10, 5);
    }

    // you're connected now, so print out the status:
    printWiFiStatus();
}

void testWiFiConnection() {
    int wifiStatus = WiFi.status();
    if (wifiStatus == WL_CONNECTION_LOST || wifiStatus == WL_DISCONNECTED || wifiStatus == WL_SCAN_COMPLETED) {
        Log.errorln("Lost connection with WiFi network, reconnecting ...");
        connectToWiFi();
    }
}


void setup() {
    pinMode(A[1], INPUT_PULLUP);
    pinMode(A[2], INPUT_PULLUP);
    pinMode(A[3], INPUT_PULLUP);
    pinMode(A[4], INPUT_PULLUP);
    WiFiDrv::pinMode(RED, OUTPUT);
    WiFiDrv::pinMode(GREEN, OUTPUT);
    WiFiDrv::pinMode(BLUE, OUTPUT);

    showColor(10, 0, 0);

    //Initialize serial and wait for port to open:
    Serial.begin(9600);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial);
//    while (!Serial) { ; // wait for serial port to connect. Needed for native USB port only
//    }

    // check for the WiFi module:
    while (WiFi.status() == WL_NO_MODULE) {
        Log.noticeln("Communication with WiFi module failed!");
    }

    String fv = WiFiClass::firmwareVersion();
    if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
        Log.noticeln("Please upgrade the firmware");
    }

    // attempt to connect to WiFi network:
    connectToWiFi();

    // set device's details (optional)
    device.setName(DEVICE_NAME);
    device.setSoftwareVersion("1.0.0");

    relay1.onCommand(onSwitchCommand);
    relay1.setName(RELAY_1);
    mrelay1.onCommand(onSwitchCommand);
    mrelay1.setName(MOMENTARY_RELAY_1);
    relay2.onCommand(onSwitchCommand);
    relay2.setName(RELAY_2);
    mrelay2.onCommand(onSwitchCommand);
    mrelay2.setName(MOMENTARY_RELAY_2);

    for (int i = 1; i < 5; i++) {
        sensors[i]->setCurrentState(sensorStates[i]);
        sensors[i]->setName(sensorNames[i]);
    }

    mqtt.begin(BROKER_ADDR, BROKER_USERNAME, BROKER_PASSWORD);
}

unsigned long connectionLastCheckedAt = 0;

void loop() {
    if (!mqtt.isConnected()) {
        Log.errorln("not connected to mqtt :(");
        showColor(10, 5, 10);
    } else {
        showColor(0, 10, 0);
    }

    mqtt.loop();

    if ((millis() - lastReadAt) > 30) {
        for (int i = 1; i < 5; i++) {
            sensors[i]->setState(digitalRead(A[i]));
            sensorStates[i] = sensors[i]->getCurrentState();
        }
        lastReadAt = millis();
    }

    momentaryRelay1.loop();
    momentaryRelay2.loop();

    if ((millis() - connectionLastCheckedAt) > 3000) {
        testWiFiConnection();
        connectionLastCheckedAt = millis();
    }
}