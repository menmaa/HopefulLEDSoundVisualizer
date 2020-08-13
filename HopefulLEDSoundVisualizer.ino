#include <math.h>

#define LED_AMOUNT 12     //The Amount of LEDs that will be used by the visualizer.

void setup() {
  Serial.begin(9600);
  
  for(int i = 2; i < 2 + LED_AMOUNT; i++) {
    pinMode(i, OUTPUT);
  }
}

void loop() {
  while(Serial.available() > 0) {
    float peakValue = Serial.read();
    int ledsOn = round((peakValue / 100) * LED_AMOUNT);
    
    for(int i = 2; i < 2 + ledsOn; i++) {
      digitalWrite(i, HIGH);
    }

    for(int i = 2 + ledsOn; i < 2 + LED_AMOUNT; i++) {
      digitalWrite(i, LOW);
    }
  }
}
