#include <Arduino.h>
#include "chrBtn.h"

chrBtn btnA("A", 4);
chrBtn btnB("B", 5);

void onButtonEvent(const char* id, uint8_t pin, chrBtn::EventCode code, uint16_t longPressCounter) {
    Serial.printf("[%s/GPIO%u] %s (%u)\n", id, pin, chrBtn::eventName(code), longPressCounter);
}

void setup() {
    Serial.begin(115200);
    chrBtn::setEventCallback(onButtonEvent);
    btnA.setup();
    btnB.setup();
}

void loop() {
    btnA.loop();
    btnB.loop();
}
