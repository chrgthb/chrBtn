# chrBtn

`chrBtn` is a small, non-blocking button library for the Arduino framework (developed for ESP32). It supports digital and analog (resistor ladder) buttons, debouncing, short/long press detection, and delivers all events to a single callback together with the button ID and GPIO number.

## Features

- Active-low (internal pull-up) and active-high buttons.
- Analog buttons: pressed while `analogRead()` is within a given range, so several buttons can share one ADC pin (resistor ladder).
- Debouncing and long-press detection with repeated hold events.
- One static callback for all buttons; each button has an ID of up to 5 characters.
- No blocking calls; `loop()` is called from the Arduino `loop()`.

## Quick Start

```cpp
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
```

The full example is in [examples/Basic/Basic.cpp](examples/Basic/Basic.cpp).

## Constructors

```cpp
chrBtn(const char* id, uint8_t pin, uint16_t longPressMs = 750, uint8_t debounceMs = 50, bool activeLow = true);

static chrBtn analog(const char* id, uint8_t pin, uint16_t minVal, uint16_t maxVal,
                     uint16_t longPressMs = 750, uint8_t debounceMs = 50);
```

| Parameter | Meaning |
| --- | --- |
| `id` | Button ID, truncated to 5 characters |
| `pin` | GPIO number |
| `longPressMs` | Time until a press counts as long; hold events repeat every third of it |
| `debounceMs` | The state must be stable for this long; `0` disables debouncing |
| `activeLow` | `true`: pressed reads LOW (internal pull-up). `false`: pressed reads HIGH (internal pull-down where available, otherwise add an external one) |
| `minVal`, `maxVal` | Inclusive `analogRead()` range of an analog button |

## Resistor ladder

Several buttons can share one ADC pin, each with its own range:

```cpp
chrBtn up  = chrBtn::analog("UP",   34, 300, 500);
chrBtn down = chrBtn::analog("DOWN", 34, 800, 1000);
```

Leave gaps between the ranges. On ESP32, ADC2 pins cannot be used while WiFi is active. To find the right ranges, use the calibration sketch in [examples/Test/Test.cpp](examples/Test/Test.cpp) (`pio run -e ladder_test -t upload`): set the pin and the number of buttons, press the buttons as asked on the serial monitor, and it suggests ranges that are refined after every round.

## Events

| Code | Value | Sent when |
| --- | ---: | --- |
| `EVENT_OK` | 0 | `setup()` finished |
| `EVENT_DOWN` | 20 | Button pressed (after debounce) |
| `EVENT_LONG_HOLD` | 40 | Held longer than `longPressMs`, then repeated; `longPressCounter` counts them |
| `EVENT_SHORT_PRESSED` | 30 | Released before the long-press time |
| `EVENT_LONG_PRESSED` | 50 | Released after a long hold |

`EVENT_ERR`, `EVENT_WARN` and `EVENT_NOTICE` are reserved. `chrBtn::eventName(code)` returns a readable name.

The callback is static and shared by all buttons: `void(const char* id, uint8_t pin, EventCode code, uint16_t longPressCounter)`.

## Other methods

- `bool loop()` returns `true` on a press or release event.
- `bool isDown()`, `bool isLongPressed()`, `uint16_t getLongPressCounter()` report the current state.
- `const char* getId()` returns the button ID.
- `enabled = false` ignores new presses; a press in progress is still completed.

## Build

```sh
pio run
```

## License

MIT. See [LICENSE](LICENSE).
