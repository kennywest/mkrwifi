#include <ArduinoHA.h>

class RelayState {
public:
    bool activated = false;
    unsigned long activatedOn = 0l;

    void activate() {
        this->activated = true;
        this->activatedOn = millis();
    }

    void deactivate() {
        this->activated = false;
        this->activatedOn = 0;
    }
};