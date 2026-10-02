// Resistor-ladder calibration helper.
// Set LADDER_PIN and NUM_BUTTONS, open the serial monitor (115200) and press the
// buttons one by one when asked. Repeat the rounds; the suggested ranges are
// refined after every complete round. Send 'r' to restart the measurement.
#include <Arduino.h>

constexpr uint8_t  LADDER_PIN  = 34;
constexpr uint8_t  NUM_BUTTONS = 4;
constexpr uint16_t ADC_MAX     = 4095;

constexpr int DETECT_DELTA = 100;  // deviation from idle that counts as a press
constexpr int MIN_GAP      = 150;  // smaller gap between neighbouring bands is reported as risky
constexpr int EDGE_PAD     = 100;  // margin added beyond the outermost bands
constexpr uint16_t SETTLE_MS  = 50;
constexpr uint16_t RELEASE_MS = 40;
constexpr uint16_t MIN_SAMPLES = 10;

struct Band {
    int mn = 0x7fffffff;
    int mx = -1;
    int64_t sum = 0;
    uint32_t cnt = 0;
    void add(int v) { if (v < mn) mn = v; if (v > mx) mx = v; sum += v; ++cnt; }
    int mean() const { return cnt ? (int)(sum / (int64_t)cnt) : 0; }
};

Band idleBand;
Band bands[NUM_BUTTONS];
uint16_t rounds = 0;

int readAdc() { return analogRead(LADDER_PIN); }

bool isPressed() { return abs(readAdc() - idleBand.mean()) > DETECT_DELTA; }

void measureIdle() {
    Serial.println(F("\nDo not press any button, measuring idle level..."));
    idleBand = Band();
    delay(1000);
    for (int i = 0; i < 200; ++i) { idleBand.add(readAdc()); delay(5); }
    Serial.printf("Idle: %d (min %d, max %d)\n", idleBand.mean(), idleBand.mn, idleBand.mx);
}

// Waits for a press, samples it until release; returns false if the press was too short.
bool capturePress(Band& out) {
    while (!isPressed()) delay(2);
    delay(SETTLE_MS);

    out = Band();
    uint32_t releasedSince = 0;
    while (true) {
        int v = readAdc();
        if (abs(v - idleBand.mean()) > DETECT_DELTA) {
            out.add(v);
            releasedSince = 0;
        } else {
            if (releasedSince == 0) releasedSince = millis();
            if (millis() - releasedSince >= RELEASE_MS) break;
        }
        delay(2);
    }
    return out.cnt >= MIN_SAMPLES;
}

void printReport() {
    // Bands sorted by mean value, idle included as a neighbour (idx -1)
    struct Entry { int idx; int mn; int mx; int mean; } e[NUM_BUTTONS + 1];
    int n = 0;
    e[n++] = { -1, idleBand.mn, idleBand.mx, idleBand.mean() };
    for (int i = 0; i < NUM_BUTTONS; ++i) e[n++] = { i, bands[i].mn, bands[i].mx, bands[i].mean() };
    for (int i = 1; i < n; ++i) {
        Entry t = e[i];
        int j = i - 1;
        while (j >= 0 && e[j].mean > t.mean) { e[j + 1] = e[j]; --j; }
        e[j + 1] = t;
    }

    Serial.printf("\n=== Round %u done: suggested ranges ===\n", rounds);
    for (int k = 0; k < n; ++k) {
        if (e[k].idx < 0) continue;

        int lo, hi;
        int gapLo = -1, gapHi = -1;
        if (k > 0) {
            gapLo = e[k].mn - e[k - 1].mx;
            lo = e[k - 1].mx + gapLo / 2;
        } else {
            lo = max(0, e[k].mn - EDGE_PAD);
        }
        if (k < n - 1) {
            gapHi = e[k + 1].mn - e[k].mx;
            hi = e[k].mx + gapHi / 2;
        } else {
            hi = min((int)ADC_MAX, e[k].mx + EDGE_PAD);
        }

        Serial.printf("B%d: measured %d..%d (avg %d, spread %d)  ->  range %d..%d",
                      e[k].idx + 1, e[k].mn, e[k].mx, e[k].mean, e[k].mx - e[k].mn, lo, hi);
        int minGap = (gapLo >= 0 && gapHi >= 0) ? min(gapLo, gapHi) : max(gapLo, gapHi);
        if ((gapLo >= 0 && gapLo < MIN_GAP) || (gapHi >= 0 && gapHi < MIN_GAP)) {
            Serial.printf("  !! gap to neighbour only %d (<%d)", minGap, MIN_GAP);
        }
        Serial.println();
    }

    Serial.println(F("\nCode:"));
    for (int k = 0; k < n; ++k) {
        if (e[k].idx < 0) continue;
        int lo = (k > 0) ? e[k - 1].mx + (e[k].mn - e[k - 1].mx) / 2 : max(0, e[k].mn - EDGE_PAD);
        int hi = (k < n - 1) ? e[k].mx + (e[k + 1].mn - e[k].mx) / 2 : min((int)ADC_MAX, e[k].mx + EDGE_PAD);
        Serial.printf("chrBtn::analog(\"B%d\", %u, %d, %d)\n", e[k].idx + 1, LADDER_PIN, lo, hi);
    }
    Serial.println(F("\nPress the buttons again to refine (send 'r' to restart)."));
}

void restart() {
    for (int i = 0; i < NUM_BUTTONS; ++i) bands[i] = Band();
    rounds = 0;
    measureIdle();
}

void setup() {
    Serial.begin(115200);
    delay(500);
    analogSetPinAttenuation(LADDER_PIN, ADC_11db);
    pinMode(LADDER_PIN, INPUT);
    restart();
}

void loop() {
    for (int i = 0; i < NUM_BUTTONS; ++i) {
        while (true) {
            while (Serial.available()) {
                if (Serial.read() == 'r') { restart(); i = 0; }
            }
            Serial.printf("Press button %d of %d...\n", i + 1, NUM_BUTTONS);
            Band press;
            if (!capturePress(press)) {
                Serial.println(F("Too short, try again."));
                continue;
            }
            bands[i].add(press.mn);
            bands[i].add(press.mx);
            bands[i].sum += press.sum - press.mn - press.mx;
            bands[i].cnt += press.cnt - 2;
            Serial.printf("  B%d: %d..%d (avg %d)\n", i + 1, press.mn, press.mx, press.mean());
            break;
        }
    }
    ++rounds;
    printReport();
}
