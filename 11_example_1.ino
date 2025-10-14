#include <Servo.h>

// Arduino pin assignment
#define PIN_LED   9   // LED active-low (LOW 신호에 켜짐)
#define PIN_TRIG  12  // sonar sensor TRIGGER
#define PIN_ECHO  13  // sonar sensor ECHO
#define PIN_SERVO 10  // servo motor

// =================================================================
// Filter Configuration
// =================================================================
// [Median Filter] 3, 5, 7 등 홀수로 설정하여 테스트하세요.
#define WINDOW_SIZE 3
// [EMA Filter] 0.0 ~ 1.0 사이로 조절하세요. (값이 작을수록 부드러워짐)
#define _EMA_ALPHA 0.3
// =================================================================

// Configurable parameters
#define SND_VEL 346.0     // sound velocity at 24 celsius degree (unit: m/sec)
#define INTERVAL 25       // sampling interval (unit: msec)
#define PULSE_DURATION 10 // ultra-sound Pulse Duration (unit: usec)
#define _DIST_MIN 180.0   // minimum distance to be measured (unit: mm)
#define _DIST_MAX 360.0   // maximum distance to be measured (unit: mm)

#define TIMEOUT ((INTERVAL / 2) * 1000.0) // maximum echo waiting time (unit: usec)
#define SCALE (0.001 * 0.5 * SND_VEL) // coefficent to convert duration to distance

// duty duration for myservo.writeMicroseconds()
#define _DUTY_MIN 1000 // servo full clockwise position (0 degree)
#define _DUTY_NEU 1500 // servo neutral position (90 degree)
#define _DUTY_MAX 2000 // servo full counterclockwise position (180 degree)

// Global variables for Filters
float samples[WINDOW_SIZE];   // Median Filter
int sample_index = 0;         // Median Filter
float dist_prev = _DIST_MAX;  // Range Filter
float dist_ema = _DIST_MAX;   // EMA Filter

// Global variables
unsigned long last_sampling_time; // unit: ms
Servo myservo;

// Function to get a distance reading from USS (unit: mm)
float USS_measure(int TRIG, int ECHO) {
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(PULSE_DURATION);
  digitalWrite(TRIG, LOW);
  return pulseIn(ECHO, HIGH, TIMEOUT) * SCALE;
}

// Median Filter Function
float median_filter(float new_sample) {
  samples[sample_index] = new_sample;
  sample_index = (sample_index + 1) % WINDOW_SIZE;

  float sorted_samples[WINDOW_SIZE];
  for (int i = 0; i < WINDOW_SIZE; i++) {
    sorted_samples[i] = samples[i];
  }

  for (int i = 0; i < WINDOW_SIZE - 1; i++) {
    for (int j = 0; j < WINDOW_SIZE - i - 1; j++) {
      if (sorted_samples[j] > sorted_samples[j + 1]) {
        float temp = sorted_samples[j];
        sorted_samples[j] = sorted_samples[j + 1];
        sorted_samples[j + 1] = temp;
      }
    }
  }
  return sorted_samples[WINDOW_SIZE / 2];
}

void setup() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);
  digitalWrite(PIN_LED, HIGH);

  myservo.attach(PIN_SERVO);
  Serial.begin(57600);

  // 모든 필터의 초기값을 안정화시킵니다.
  float first_sample = USS_measure(PIN_TRIG, PIN_ECHO);
  if (first_sample <= 0 || first_sample < _DIST_MIN || first_sample > _DIST_MAX) {
    first_sample = _DIST_MAX; // 첫 측정이 불안정하면 최대값으로 설정
  }
  
  for (int i = 0; i < WINDOW_SIZE; i++) {
    samples[i] = first_sample;
  }
  dist_prev = first_sample;
  dist_ema = first_sample;
  
  myservo.writeMicroseconds(_DUTY_MAX); // 초기 위치를 180도로 설정
}

void loop() {
  if (millis() < last_sampling_time + INTERVAL)
    return;

  // --- 데이터 처리 파이프라인 ---
  
  // 1. Raw Data 측정
  float dist_raw = USS_measure(PIN_TRIG, PIN_ECHO);

  // 2. 범위 필터 (Range Filter) 적용
  float dist_ranged;
  if (dist_raw <= 0 || dist_raw < _DIST_MIN || dist_raw > _DIST_MAX) {
    dist_ranged = dist_prev;
    digitalWrite(PIN_LED, HIGH); // Turn LED OFF
  } else {
    dist_ranged = dist_raw;
    dist_prev = dist_raw;
    digitalWrite(PIN_LED, LOW); // Turn LED ON
  }

  // 3. 중앙값 필터 (Median Filter) 적용
  float dist_median = median_filter(dist_ranged);

  // 4. EMA 필터 적용 (최종 값)
  dist_ema = _EMA_ALPHA * dist_median + (1 - _EMA_ALPHA) * dist_ema;
  
  // ------------------------------

  // 거리에 비례한 서보 모터 제어 (최종 필터링된 dist_ema 값 사용)
  int duty;
  if (dist_ema <= _DIST_MIN) {
    duty = _DUTY_MIN;
  } else if (dist_ema >= _DIST_MAX) {
    duty = _DUTY_MAX;
  } else {
    duty = map(dist_ema, _DIST_MIN, _DIST_MAX, _DUTY_MIN, _DUTY_MAX);
  }
  myservo.writeMicroseconds(duty);

  // 시리얼 출력 (원본 값과 필터링된 최종 값을 비교)
  Serial.print("Raw(mm):"); Serial.print(dist_raw);
  Serial.print(", Final(mm):"); Serial.print(dist_ema);
  Serial.print(", Servo Duty:"); Serial.print(duty);
  Serial.println("");
  
  last_sampling_time += INTERVAL;
}
