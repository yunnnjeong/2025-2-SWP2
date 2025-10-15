#include <Servo.h>
#include <math.h>

#define PIN_SERVO 10
#define PIN_TRIG 12
#define PIN_ECHO 13
#define INTERVAL 25      // ms
#define SND_VEL 346.0    // m/s
#define PULSE_DURATION 10 // us
#define TIMEOUT ((INTERVAL / 2) * 1000.0)
#define SCALE (0.001 * 0.5 * SND_VEL)

// 차단기 각도 정의
const int DOWN_ANGLE = 90;
const int UP_ANGLE   = 0;

Servo myServo;
unsigned long last_sampling_time = 0;

// 움직임 상태 저장 변수
bool isMoving = false;
unsigned long moveStartTime;
unsigned long MOVING_TIME = 2000; // 2초 동안 움직임
int fromAngle = DOWN_ANGLE;
int toAngle   = DOWN_ANGLE;

// Ease-in-out 함수 (Quadratic)
// 0.0 ~ 1.0 사이의 입력 t를 받아 0.0 ~ 1.0 사이의 S자 곡선 값을 반환
float easeInOut(float t) {
  if (t < 0.5f) {
    return 2.0f * t * t;
  } else {
    return 1.0f - pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
  }
}

// 초음파 거리측정 함수
float USS_measure(int TRIG, int ECHO) {
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(PULSE_DURATION);
  digitalWrite(TRIG, LOW);
  long duration = pulseIn(ECHO, HIGH, TIMEOUT);
  if (duration == 0) return -1;
  return duration * SCALE;
}

void setup() {
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);
  myServo.attach(PIN_SERVO);
  myServo.write(DOWN_ANGLE);
  Serial.begin(115200);
}

void loop() {
  if (isMoving) {
    unsigned long now = millis();
    unsigned long progress = now - moveStartTime;
    if (progress <= MOVING_TIME) {
      // --- 이 부분이 변경되었습니다 ---
      // 1. 진행률(ratio)을 0.0 ~ 1.0 사이 값으로 직접 계산
      float ratio_progress = (float)progress / MOVING_TIME;
      // 2. easeInOut 함수를 사용하여 S자 곡선 비율 계산
      float ratio = easeInOut(ratio_progress);
      
      int angle = fromAngle + (toAngle - fromAngle) * ratio;
      myServo.write(angle);
    } else {
      myServo.write(toAngle);
      fromAngle = toAngle;
      isMoving = false;
    }
    return;
  }

  // 2. 센서 주기에 따라 측정/명령만 수행
  if (millis() >= last_sampling_time + INTERVAL) {
    last_sampling_time = millis();
    float distance = USS_measure(PIN_TRIG, PIN_ECHO);
    Serial.print("Distance: ");
    Serial.println(distance);

    // 3. '목표각도와 현재각도가 다를 때'만 새로운 동작 시작
    // 차량 접근: 차단기 올리기
    if (distance >= 200.0 && distance <= 500.0 && toAngle != UP_ANGLE) {
      fromAngle = myServo.read();
      toAngle = UP_ANGLE;
      moveStartTime = millis();
      isMoving = true;
    }
    // 차량 매우 가까움: 차단기 내리기
    else if (distance > 0 && distance <= 150.0 && toAngle != DOWN_ANGLE) {
      fromAngle = myServo.read();
      toAngle = DOWN_ANGLE;
      moveStartTime = millis();
      isMoving = true;
    }
  }
}
