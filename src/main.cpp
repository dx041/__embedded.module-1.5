#include "HardwareSerial.h"
#include "esp32-hal-gpio.h"
#include "esp32-hal.h"
#include "freertos/portmacro.h"
#include <Arduino.h>
#include <cstdint>


#define RAW_DATA_PIN 38
#define CLEAN_DATA_PIN 40
#define EXTERNAL_BTN_PIN 21

void onExternalBtnClick();
void print_F();


const uint32_t DEBOUNCE_MS = 30;

uint8_t stableValue = HIGH;
uint32_t stablePressedCount = 0;

volatile uint8_t lastRawValue = HIGH;
volatile uint32_t rawPressedCount = 0;
volatile uint32_t lastRawChangeMs_V = 0;
volatile uint32_t interraptCount = 0;

portMUX_TYPE btnMutex = portMUX_INITIALIZER_UNLOCKED;


void setup() {
    Serial.begin(115200);
    pinMode(EXTERNAL_BTN_PIN, INPUT); // external pull_up

    pinMode(RAW_DATA_PIN, OUTPUT);
    pinMode(CLEAN_DATA_PIN, OUTPUT);

    lastRawValue = digitalRead(EXTERNAL_BTN_PIN);
    stableValue = lastRawValue;
    lastRawChangeMs_V = millis();

    digitalWrite(RAW_DATA_PIN, lastRawValue);
    digitalWrite(CLEAN_DATA_PIN, stableValue);

    attachInterrupt(digitalPinToInterrupt(EXTERNAL_BTN_PIN), onExternalBtnClick, CHANGE);
}

void loop() {
    portENTER_CRITICAL(&btnMutex);
    const uint32_t now = millis();
    const uint8_t raw = lastRawValue;
    const uint32_t lastChange_MS = lastRawChangeMs_V;
    const uint32_t rawCount = rawPressedCount;
    portEXIT_CRITICAL(&btnMutex);

    digitalWrite(RAW_DATA_PIN, raw);

    if (raw != stableValue) {
        if ((millis() - lastChange_MS) >= DEBOUNCE_MS) {
            stableValue = raw;
            if (stableValue == LOW) {
                stablePressedCount++;
            }
        }
    }

    digitalWrite(CLEAN_DATA_PIN, stableValue);

    static uint32_t lastReportMs = 0;
    if ((millis() - lastReportMs) >= 1000U) {
        lastReportMs = now;
        print_F();
    }
}

void ARDUINO_ISR_ATTR onExternalBtnClick() {
    portENTER_CRITICAL_ISR(&btnMutex);
    
    const uint32_t raw = digitalRead(EXTERNAL_BTN_PIN);
    lastRawChangeMs_V = millis();
    if (raw == LOW) {
        interraptCount++;
        if (raw != lastRawValue) {
            rawPressedCount++;
        }
    }
    lastRawValue = raw;

    portEXIT_CRITICAL_ISR(&btnMutex);
}

void print_F() {
    Serial.printf("Raw: %u | Debounced: %u | IRS-Calls: %u\n",
        static_cast<uint32_t>(rawPressedCount),
        static_cast<uint32_t>(stablePressedCount),
        static_cast<uint32_t>(interraptCount)
    );
}