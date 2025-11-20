#include <Arduino.h>

#define LED 8

void setup() {
    Serial.begin(115200);
    Serial.println("Hello World");
    pinMode(LED, OUTPUT);
}

void loop() {
    
    digitalWrite(LED, HIGH);
    delay(100);
    digitalWrite(LED, LOW);
    delay(100);
}

