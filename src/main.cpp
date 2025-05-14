#include <Arduino.h>
#include <PinChangeInterrupt.h>
#include <math.h>

// 함수 선언만 먼저 해줍니다 (본문 없이 세미콜론 붙이기)
void ch1ISR();
void ch2ISR();
void ch8ISR();
void hsvToRgb(float h, float s, float v, float* r, float* g, float* b);


//CH1: 밝기 조절 (D2), CH2: 색상 조절 (D3), CH8: ON/OFF (D8)
int ch1Pin = 2;
int ch2Pin = 3;
int ch8Pin = 8;

// 일반 LED 제어용 핀 (밝기 조절용)
int ledPins[3] = {9, 10, 11};

// RGB 삼색 LED 제어용 핀
int rgbPins[3] = {5, 6, 7};

// PWM 측정용 변수들
volatile unsigned long ch1Start, ch2Start, ch8Start;  // HIGH 시각 기록
volatile int ch1Value = 1500, ch2Value = 1500, ch8Value = 1000;  // 펄스 폭 저장 (us 단위)

void setup() {
  Serial.begin(9600);  // 시리얼 모니터 시작

  // 각 채널 핀을 입력으로 설정
  pinMode(ch1Pin, INPUT);
  pinMode(ch2Pin, INPUT);
  pinMode(ch8Pin, INPUT);

  // 핀체인지 인터럽트 연결
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(ch1Pin), ch1ISR, CHANGE);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(ch2Pin), ch2ISR, CHANGE);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(ch8Pin), ch8ISR, CHANGE);

  // 일반 LED 및 RGB LED 핀 출력으로 설정
  for (int i = 0; i < 3; i++) {
    pinMode(ledPins[i], OUTPUT);
    pinMode(rgbPins[i], OUTPUT);
  }
}

void loop() {
  // CH1 → 밝기: 1000~2000us → 0~255 밝기 값으로 변환
  int brightness = map(ch1Value, 1000, 2000, 0, 255);
  brightness = constrain(brightness, 0, 255);  // 0~255 범위로 제한

  // CH2 → 색상 조절: 1000~2000us → 0~180도 hue로 변환
  float hue = map(ch2Value, 1000, 2000, 0, 180);
  float r, g, b;

  // HSV to RGB 변환
  hsvToRgb(hue, 1.0, 1.0, &r, &g, &b);  // 채도/명도는 1.0 고정

  // CH8 → ON/OFF 제어: 기준값 1400us
  bool ledOn = ch8Value > 1400;

  // 일반 LED 출력 (On이면 밝기 적용, Off면 0)
  for (int i = 0; i < 3; i++) {
    analogWrite(ledPins[i], ledOn ? brightness : 0);
  }

  // 삼색 RGB LED 출력
  analogWrite(rgbPins[0], ledOn ? int(r * brightness) : 0);
  analogWrite(rgbPins[1], ledOn ? int(g * brightness) : 0);
  analogWrite(rgbPins[2], ledOn ? int(b * brightness) : 0);

  // 시리얼 출력 (디버깅용)
  Serial.print("CH1(brightness): "); Serial.print(ch1Value);
  Serial.print(" | CH2(hue): "); Serial.print(hue);
  Serial.print(" | CH8(switch): "); Serial.println(ledOn ? "ON" : "OFF");

  delay(100);  // 0.1초 간격으로 업데이트
}

// === CH1 인터럽트 핸들러: 밝기 조절 ===
void ch1ISR() {
  if (digitalRead(ch1Pin) == HIGH) {
    ch1Start = micros();  // 상승 엣지에서 시간 기록
  } else {
    ch1Value = micros() - ch1Start;  // 펄스 길이 계산
  }
}

// === CH2 인터럽트 핸들러: 색상 조절 ===
void ch2ISR() {
  if (digitalRead(ch2Pin) == HIGH) {
    ch2Start = micros();
  } else {
    ch2Value = micros() - ch2Start;
  }
}

// === CH8 인터럽트 핸들러: On/Off 제어 ===
void ch8ISR() {
  if (digitalRead(ch8Pin) == HIGH) {
    ch8Start = micros();
  } else {
    ch8Value = micros() - ch8Start;
  }
}

// === HSV to RGB 변환 함수 ===
// h: 색상(Hue, 0~360), s: 채도(Saturation), v: 명도(Value)
// r/g/b 포인터로 결과 리턴 (0.0~1.0 범위)
void hsvToRgb(float h, float s, float v, float* r, float* g, float* b) {
  int i = int(h / 60.0) % 6;
  float f = h / 60.0 - i;
  float p = v * (1 - s);
  float q = v * (1 - f * s);
  float t = v * (1 - (1 - f) * s);

  switch (i) {
    case 0: *r = v; *g = t; *b = p; break;
    case 1: *r = q; *g = v; *b = p; break;
    case 2: *r = p; *g = v; *b = t; break;
    case 3: *r = p; *g = q; *b = v; break;
    case 4: *r = t; *g = p; *b = v; break;
    case 5: *r = v; *g = p; *b = q; break;
  }
}
