#include <Ethernet.h>
#include <ArduinoHA.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <utility/wifi_drv.h>

#include "arduino_secrets.h"

#define DEVICE_NAME "mkr_wifi_1010_01"

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
char RELAY_1[] = "relay1";
char RELAY_2[] = "relay2";
char A_1[] = "digital_a1";
char A_2[] = "digital_a2";
char A_3[] = "digital_a3";
char A_4[] = "digital_a4";

int status = WL_IDLE_STATUS;

int a[] = {15, 16, 17, 18, 19};
char *sensorNames[]{nullptr, A_1, A_2, A_3, A_4};
boolean sensorStates[] = {LOW, LOW, LOW, LOW, LOW};
unsigned long lastReadAt = millis();

int relay[] = {0, 1, 2};

byte mac[] = {0x24, 0x0a, 0xc4, 0xc3, 0xc5, 0x24};

WiFiClient client;

HADevice device(mac, sizeof(mac));
HAMqtt mqtt(client, device, 20);

HASwitch relay1(RELAY_1);
HASwitch relay2(RELAY_2);
HABinarySensor *a1 = new HABinarySensor(sensorNames[1]);
HABinarySensor *a2 = new HABinarySensor(sensorNames[2]);
HABinarySensor *a3 = new HABinarySensor(sensorNames[3]);
HABinarySensor *a4 = new HABinarySensor(sensorNames[4]);
HABinarySensor *sensors[] = {nullptr, a1, a2, a3, a4};

void onSwitchCommand(bool state, HASwitch *sender) {
    if (sender == &relay1) {
        digitalWrite(relay[1], state ? HIGH : LOW);
    }
    if (sender == &relay2) {
        digitalWrite(relay[2], state ? HIGH : LOW);
    }

    sender->setState(state); // report state back to the Home Assistant
}

void printWifiStatus() {
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your board's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}

void setup() {
    pinMode(a[1], INPUT_PULLUP);
    pinMode(a[2], INPUT_PULLUP);
    pinMode(a[3], INPUT_PULLUP);
    pinMode(a[4], INPUT_PULLUP);

    //Initialize serial and wait for port to open:
    Serial.begin(9600);
    while (!Serial) { ; // wait for serial port to connect. Needed for native USB port only
    }

    // check for the WiFi module:
    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println("Communication with WiFi module failed!");
        // don't continue
        while (true);
    }

    String fv = WiFiClass::firmwareVersion();
    if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
        Serial.println("Please upgrade the firmware");
    }

    // attempt to connect to WiFi network:
    while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(ssid);
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        status = WiFi.begin(ssid, pass);

        // wait 10 seconds for connection:
        delay(10000);
    }

    // you're connected now, so print out the status:
    printWifiStatus();


    // set device's details (optional)
    device.setName(DEVICE_NAME);
    device.setSoftwareVersion("1.0.0");

    relay1.onCommand(onSwitchCommand);
    relay1.setName(RELAY_1);
    relay2.onCommand(onSwitchCommand);
    relay2.setName(RELAY_2);

    for (int i = 1; i < 5; i++) {
        sensors[i]->setCurrentState(sensorStates[i]);
        sensors[i]->setName(sensorNames[i]);
    }

    mqtt.begin(BROKER_ADDR, BROKER_USERNAME, BROKER_PASSWORD);
}

void loop() {
    if (!mqtt.isConnected()) {
        Serial.println("not connected to mqtt :(");
    }

    mqtt.loop();

    if ((millis() - lastReadAt) > 30) {
        for (int i = 1; i < 5; i++) {
            sensors[i]->setState(digitalRead(a[i]));
            sensorStates[i] = sensors[i]->getCurrentState();
        }
        lastReadAt = millis();
    }
}