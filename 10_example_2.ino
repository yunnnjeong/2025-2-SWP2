#include <Servo.h>
#define PIN_SERVO 10

Servo myservo;

void setup() {
  myservo.attach(PIN_SERVO); 
  myservo.write(0);
  delay(1000);
}

void loop() {
    // add code here.
}
