#pragma once

#include <Arduino.h>

class chrBtn {
public:
    static constexpr uint8_t ID_MAX_LEN = 5;

    // Event codes
    enum EventCode {
        EVENT_ERR           = -10,
        EVENT_WARN          = -1,
        EVENT_OK            = 0,
        EVENT_NOTICE        = 10,
        EVENT_DOWN          = 20,
        EVENT_SHORT_PRESSED = 30,
        EVENT_LONG_HOLD     = 40,
        EVENT_LONG_PRESSED  = 50
    };

    // id: button identifier, truncated to ID_MAX_LEN characters
    using EventCallback = void (*)(const char* id, uint8_t pin, EventCode code, uint16_t longPressCounter);

    bool enabled = true;
    
    // activeLow: true = pressed reads LOW (internal pull-up), false = pressed reads HIGH (pull-down, if the core supports it)
    chrBtn(const char* id, uint8_t pin, uint16_t longPressMs = 750, uint8_t debounceMs = 50, bool activeLow = true);

    // Analog button (e.g. resistor ladder): pressed while analogRead(pin) is within [minVal, maxVal]
    static chrBtn analog(const char* id, uint8_t pin, uint16_t minVal, uint16_t maxVal,
                         uint16_t longPressMs = 750, uint8_t debounceMs = 50);

    void setup();
    static void setEventCallback(EventCallback cb);
    static const char* eventName(EventCode code);
    const char* getId() const;
    bool loop();
    bool isDown();
    bool isLongPressed();
    uint16_t getLongPressCounter();
    
private:
    static EventCallback _onEvent;
    char _id[ID_MAX_LEN + 1];
    uint8_t _pin;
    uint16_t _longPressMs;
    uint8_t _debounceMs;
    bool _activeLow;
    bool _analog = false;
    uint16_t _anaMin = 0;
    uint16_t _anaMax = 0;
    unsigned long _firstDownMillis = 0;
    unsigned long _lastHoldEventMillis = 0;
    unsigned long _lastDebounceMillis = 0;
    
    bool _isDown = false;
    uint16_t _longPressCounter = 0;
    bool _lastState = false; // last sampled pressed state

    bool _sample();
    void _fireEvent(EventCode code);
};
