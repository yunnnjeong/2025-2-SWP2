// Arduino pin assignment
#define PIN_LED  9
#define PIN_TRIG 12   // sonar sensor TRIGGER
#define PIN_ECHO 13   // sonar sensor ECHO

// configurable parameters
#define SND_VEL 346.0     // sound velocity at 24 celsius degree (unit: m/sec)
#define INTERVAL 25       // sampling interval (unit: msec)
#define PULSE_DURATION 10 // ultra-sound Pulse Duration (unit: usec)
#define _DIST_MIN 100.0   // minimum distance to be measured (unit: mm)
#define _DIST_MAX 300.0   // maximum distance to be measured (unit: mm)

#define TIMEOUT ((INTERVAL / 2) * 1000.0) // maximum echo waiting time (unit: usec)
#define SCALE (0.001 * 0.5 * SND_VEL)     // coefficient to convert duration to distance

unsigned long last_sampling_time;   // unit: msec

float distance = _DIST_MAX;

void setup() {
  // initialize GPIO pins
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);  // sonar TRIGGER
  pinMode(PIN_ECHO, INPUT);   // sonar ECHO
  digitalWrite(PIN_TRIG, LOW);  // turn-off Sonar 
  
  // initialize serial port
  Serial.begin(115200);
}

void loop() { 
  // wait until next sampling time. // polling
  if (millis() < (last_sampling_time + INTERVAL))
    return;

  // ---------- LED 밝기 제어 ----------
  int brightness = 0;
  if ((distance == 0.0) || (distance > _DIST_MAX) || (distance < _DIST_MIN)) {
    brightness = 0;   // 범위 밖 → LED 꺼짐
  } else if (distance <= 200.0) {
    // 100mm~200mm : 선형적으로 밝기 증가 (0 → 255)
    brightness = map(distance, 100, 200, 0, 255);
  } else {
    // 200mm~300mm : 선형적으로 밝기 감소 (255 → 0)
    brightness = map(distance, 200, 300, 255, 0);
  }

  analogWrite(PIN_LED, brightness);

  // ---------- 시리얼 출력 ----------
  Serial.print("Min:");        Serial.print(_DIST_MIN);
  Serial.print(", distance:"); Serial.print(distance);
  Serial.print(", Max:");      Serial.print(_DIST_MAX);
  Serial.print(", brightness:"); Serial.println(brightness);

  // ---------- 초음파 센서로 거리 갱신 ----------
  distance = USS_measure(PIN_TRIG, PIN_ECHO); // read distance
  
  // update last sampling time
  last_sampling_time += INTERVAL;
}

// get a distance reading from USS. return value is in millimeter.
float USS_measure(int TRIG, int ECHO)
{
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(PULSE_DURATION);
  digitalWrite(TRIG, LOW);
  
  return pulseIn(ECHO, HIGH, TIMEOUT) * SCALE; // unit: mm
}
