#include <Arduino.h>
#include <ArduinoHA.h>
#include "relay_state.h"

class MomentaryRelay {
private:
    HASwitch *mrelay;
    HASwitch *relay;
    int pin;
    RelayState relayState;
public:
    MomentaryRelay(HASwitch *mrelay, HASwitch *relay, int pin) {
        this->mrelay = mrelay;
        this->relay = relay;
        this->pin = pin;
    }

    void activate() {
        digitalWrite(pin, HIGH);
        mrelay->setState(true);
        relay->setState(true);
        relayState.activate();
    }

    void deactivate() {
        digitalWrite(pin, LOW);
        mrelay->setState(false);
        relay->setState(false);
        relayState.deactivate();
    }

    void loop() {
        if (relayState.activated && (millis() - relayState.activatedOn) > 1000) {
            deactivate();
        }
    }
};
