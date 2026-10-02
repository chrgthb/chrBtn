#include "chrBtn.h"

chrBtn::EventCallback chrBtn::_onEvent = nullptr;

chrBtn::chrBtn(const char* id, uint8_t pin, uint16_t longPressMs, uint8_t debounceMs, bool activeLow) {
    _activeLow = activeLow;
    strncpy(_id, id ? id : "", ID_MAX_LEN);
    _id[ID_MAX_LEN] = '\0';
    _pin = pin;
    _longPressMs = longPressMs;
    _debounceMs = debounceMs;
}

void chrBtn::setup() {
    if (_analog) {
        pinMode(_pin, INPUT);
    } else {
#ifdef INPUT_PULLDOWN
        pinMode(_pin, _activeLow ? INPUT_PULLUP : INPUT_PULLDOWN);
#else
        pinMode(_pin, _activeLow ? INPUT_PULLUP : INPUT);
#endif
    }
    _lastState = _sample();
    _fireEvent(EVENT_OK);
}

chrBtn chrBtn::analog(const char* id, uint8_t pin, uint16_t minVal, uint16_t maxVal,
                      uint16_t longPressMs, uint8_t debounceMs) {
    chrBtn b(id, pin, longPressMs, debounceMs);
    b._analog = true;
    b._anaMin = minVal;
    b._anaMax = maxVal;
    return b;
}

bool chrBtn::_sample() {
    if (_analog) {
        uint16_t v = analogRead(_pin);
        return v >= _anaMin && v <= _anaMax;
    }
    bool reading = digitalRead(_pin);
    return _activeLow ? !reading : reading;
}

void chrBtn::setEventCallback(EventCallback cb) { 
    _onEvent = cb; 
}

const char* chrBtn::getId() const {
    return _id;
}

const char* chrBtn::eventName(EventCode code) {
    switch (code) {
        case EVENT_ERR:           return "error";
        case EVENT_WARN:          return "warning";
        case EVENT_OK:            return "ok";
        case EVENT_NOTICE:        return "notice";
        case EVENT_DOWN:          return "down";
        case EVENT_SHORT_PRESSED: return "short pressed";
        case EVENT_LONG_HOLD:     return "holding down";
        case EVENT_LONG_PRESSED:  return "long pressed";
    }
    return "unknown";
}

bool chrBtn::loop() {
    if (!enabled && !_isDown) return false;

    uint32_t now = millis();
    bool btn = _sample();
  
    if (btn != _lastState) {
        _lastDebounceMillis = now;
    }
    _lastState = btn;
  
    if (_debounceMs == 0 || ( (now - _lastDebounceMillis) > _debounceMs) ) {
        // Stable state
        if (btn) {
            if (_isDown) {
                // Not the first press (button is down and the stored state is also down)
                if ( (_longPressCounter == 0) && ((now - _firstDownMillis) > _longPressMs) ) {
                    // First long press
                    _longPressCounter = 1;
                    _lastHoldEventMillis = now;
                    _fireEvent(EVENT_LONG_HOLD);
                } else if ( (_longPressCounter > 0) && ((now - _lastHoldEventMillis) > (_longPressMs / 3)) ) {
                    // Subsequent long presses, if at least one-third of the long press duration has passed
                    ++_longPressCounter;
                    _lastHoldEventMillis = now;
                    _fireEvent(EVENT_LONG_HOLD);
                }
            } else {
                // First press (button is down, but the stored state was previously up)
                _longPressCounter = 0;
                _isDown = true;
                _firstDownMillis = now;
                _fireEvent(EVENT_DOWN);
                
                return true;
            }
        } else if (_isDown) {
            // Release (button is now up, but the stored state was previously down)
            _isDown = false;
            _fireEvent(_longPressCounter > 0 ? EVENT_LONG_PRESSED : EVENT_SHORT_PRESSED);

            _longPressCounter = 0;
            return true;
        }
    }
    
    return false;
}

bool chrBtn::isDown() {
    return _isDown;
}

bool chrBtn::isLongPressed() {
    return _longPressCounter > 0;
}

uint16_t chrBtn::getLongPressCounter() {
    return _longPressCounter;
}

void chrBtn::_fireEvent(EventCode code) {
    if (_onEvent) _onEvent(_id, _pin, code, _longPressCounter);
}