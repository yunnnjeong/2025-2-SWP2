#include <Servo.h>

// Arduino pin assignment
#define PIN_LED   9
#define PIN_SERVO 10
#define PIN_IR    A0

// Event interval parameters (unit: ms)
#define _INTERVAL_DIST    20
#define _INTERVAL_SERVO   20
#define _INTERVAL_SERIAL  100

// EMA filter configuration
#define _EMA_ALPHA 0.5    // 0~1

// Servo duty values (calibrated)
#define _DUTY_MAX 2050   // lowest tilt (down)
#define _DUTY_NEU 1420   // center
#define _DUTY_MIN 900    // highest tilt (up)

#define _SERVO_ANGLE_DIFF  95.0   // measured angle range (deg)
#define _SERVO_SPEED       200   // deg/sec (95° / 1.2s)

// Target Distance (mm)
#define _DIST_TARGET    155

// PID parameters (P only)
#define _KP  2   

// -------------------------------------------------------

Servo myservo;
float dist_ema = 0;

// Event periods
unsigned long last_sampling_time_dist = 0;
unsigned long last_sampling_time_servo = 0;
unsigned long last_sampling_time_serial = 0;
bool event_dist = false;
bool event_servo = false;
bool event_serial = false;

// Servo speed control
int duty_change_per_interval;

// Servo position
int duty_target;
int duty_current;

int error_current;
float pterm;

// -------------------------------------------------------

void setup()
{
  pinMode(PIN_LED,OUTPUT);
  
  myservo.attach(PIN_SERVO);
  duty_target = duty_current = _DUTY_NEU;
  myservo.writeMicroseconds(duty_current);    

  // convert speed into duty change per interval.
  duty_change_per_interval = 
    (float)(_DUTY_MAX - _DUTY_MIN) * 
    ((float)_SERVO_SPEED / _SERVO_ANGLE_DIFF) * 
    (_INTERVAL_SERVO / 1000.0); 
  
  // initialize serial port
  Serial.begin(1000000);  
}
  
// -------------------------------------------------------

void loop()
{
  unsigned long time_curr = millis();
  
  // Event timing
  if (time_curr >= (last_sampling_time_dist + _INTERVAL_DIST)) {
    last_sampling_time_dist += _INTERVAL_DIST;
    event_dist = true;
  }
  if (time_curr >= (last_sampling_time_servo + _INTERVAL_SERVO)) {
    last_sampling_time_servo += _INTERVAL_SERVO;
    event_servo = true;
  }
  if (time_curr >= (last_sampling_time_serial + _INTERVAL_SERIAL)) {
    last_sampling_time_serial += _INTERVAL_SERIAL;
    event_serial = true;
  }

  // ---------- Distance Event ----------
  if (event_dist) {
    float dist_filtered; // unit: mm
    int control;
    
    event_dist = false;

    // get a distance reading
    int raw = ir_sensor_filtered(20, 0.5, 0);
    dist_filtered = volt_to_distance(raw);
    dist_ema = _EMA_ALPHA * dist_filtered + (1.0 - _EMA_ALPHA) * dist_ema;

    // PID (P only)
    error_current = _DIST_TARGET - dist_ema;
    pterm = _KP * error_current;
    
    control = pterm;
    duty_target = _DUTY_NEU + control;

    // Limit duty_target
    if (duty_target < _DUTY_MIN)
      duty_target = _DUTY_MIN;
    if (duty_target > _DUTY_MAX)
      duty_target = _DUTY_MAX;
      
    if (error_current > 0)
      digitalWrite(PIN_LED, 1);
    else
      digitalWrite(PIN_LED, 0);
  }
  
  // ---------- Servo Event ----------
  if (event_servo) {
    event_servo = false;
     
    // adjust duty_current toward duty_target
    if (duty_target > duty_current) {
      duty_current += duty_change_per_interval;
      if (duty_current > duty_target)
          duty_current = duty_target;
    } else {
      duty_current -= duty_change_per_interval;
      if (duty_current < duty_target)
        duty_current = duty_target;
    }

    myservo.writeMicroseconds(duty_current);
  }
  
  // ---------- Serial Event ----------
  if (event_serial) {
    event_serial = false;

    Serial.print("RAW:");
    Serial.print(analogRead(PIN_IR));
    Serial.print(",DIST:");
    Serial.print(dist_ema);
    Serial.print(",ERROR:");
    Serial.print(error_current);
    Serial.print(",TARGET:");
    Serial.print(duty_target);
    Serial.print(",CURRENT:");
    Serial.print(duty_current);  
    Serial.print(",pterm:");
    Serial.println(pterm);
  }
}

// -------------------------------------------------------
// 3rd-order polynomial (raw → mm)
// -------------------------------------------------------

float volt_to_distance(float raw)
{
    return
      0.00000351460608 * raw * raw * raw
    - 0.00290991825    * raw * raw
    - 0.171875421      * raw
    + 383.969126;
}

// -------------------------------------------------------

int compare(const void *a, const void *b) {
  return (*(unsigned int *)a - *(unsigned int *)b);
}

// Median / percentile filter for IR sensor
unsigned int ir_sensor_filtered(unsigned int n, float position, int verbose)
{
  unsigned int *ir_val, ret_val;
 
  if ((n == 0) || (n > 100) || (position < 0.0) || (position > 1))
    return 0;
    
  if (position == 1.0)
    position = 0.999;

  ir_val = (unsigned int *)malloc(sizeof(unsigned int) * n);
  if (ir_val == NULL)
    return 0;

  for (int i = 0; i < n; i++) {
    ir_val[i] = analogRead(PIN_IR);
  }

  qsort(ir_val, n, sizeof(unsigned int), compare);
  ret_val = ir_val[(unsigned int)(n * position)];

  free(ir_val);

  return ret_val;
}
