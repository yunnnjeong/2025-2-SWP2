// 아두이노 핀 할당
#define PIN_LED  9   // LED를 연결할 핀 번호
#define PIN_TRIG 12  // 초음파 센서 Trig 핀 번호
#define PIN_ECHO 13  // 초음파 센서 Echo 핀 번호

// =================================================================
// 필터 설정 값
// 이 값을 3, 10, 30으로 변경하며 필터 성능을 테스트하세요.
#define WINDOW_SIZE 10
// EMA 필터는 슬라이드 형식에만 필요하므로 ALPHA 값도 정의합니다.
#define ALPHA 0.2
// =================================================================

// 주요 파라미터 설정
#define SND_VEL 346.0
#define INTERVAL 25
#define PULSE_DURATION 10
#define _DIST_MIN 100
#define _DIST_MAX 300
#define TIMEOUT ((INTERVAL / 2) * 1000.0)
#define SCALE (0.001 * 0.5 * SND_VEL)

// 전역 변수
float samples[WINDOW_SIZE];
int sample_index = 0;
float dist_ema = 0.0; // EMA 필터 결과값 저장을 위해 필요
unsigned long last_sampling_time;

// 거리 측정 함수
float USS_measure(int TRIG, int ECHO) {
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(PULSE_DURATION);
  digitalWrite(TRIG, LOW);
  return pulseIn(ECHO, HIGH, TIMEOUT) * SCALE;
}

// 중앙값 필터 함수
float median_filter(float new_sample) {
  samples[sample_index] = new_sample;
  sample_index = (sample_index + 1) % WINDOW_SIZE;
  float sorted_samples[WINDOW_SIZE];
  for (int i = 0; i < WINDOW_SIZE; i++) { sorted_samples[i] = samples[i]; }
  // 버블 정렬 로직
  for (int i = 0; i < WINDOW_SIZE - 1; i++) {
    for (int j = 0; j < WINDOW_SIZE - i - 1; j++) {
      if (sorted_samples[j] > sorted_samples[j + 1]) {
        // 값 교환(swap) 로직 수정
        float temp = sorted_samples[j];
        sorted_samples[j] = sorted_samples[j + 1];
        sorted_samples[j + 1] = temp;
      }
    }
  }
  return sorted_samples[WINDOW_SIZE / 2];
}

// EMA 필터 함수 (출력 형식에 필요)
float ema_filter(float new_sample) {
  dist_ema = ALPHA * new_sample + (1 - ALPHA) * dist_ema;
  return dist_ema;
}


void setup() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);
  Serial.begin(57600);
  
  float first_sample = USS_measure(PIN_TRIG, PIN_ECHO);
  for (int i = 0; i < WINDOW_SIZE; i++) {
    samples[i] = first_sample;
  }
  dist_ema = first_sample;
  last_sampling_time = millis();
}

void loop() {
  if (millis() < last_sampling_time + INTERVAL) return;

  float dist_raw = USS_measure(PIN_TRIG, PIN_ECHO);

  // 각 필터 값 계산
  float current_ema = ema_filter(dist_raw);
  float dist_median = median_filter(dist_raw);

  // --- 시리얼 출력 형식 (슬라이드와 동일) ---
  Serial.print("Min:");
  Serial.print(_DIST_MIN);
  Serial.print(",raw:");
  Serial.print(dist_raw);
  Serial.print(",ema:");
  Serial.print(current_ema);
  Serial.print(",median:");
  Serial.print(dist_median);
  Serial.print(",Max:");
  Serial.print(_DIST_MAX);
  Serial.println("");

  // LED 제어는 중앙값 필터 결과를 기준으로
  if (dist_median <= _DIST_MIN || dist_median >= _DIST_MAX)
    digitalWrite(PIN_LED, LOW);
  else
    digitalWrite(PIN_LED, HIGH);

  last_sampling_time += INTERVAL;
}
