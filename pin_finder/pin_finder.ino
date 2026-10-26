/*
 * 핀 찾기 테스트 (ESP32-C3)
 *
 * 기존 브레드보드 회로에서 어떤 GPIO가 빨간불/초록불에 연결돼 있는지 찾는다.
 * 빨간불/초록불/화살표 PCB 모두 같은 방법으로 찾을 수 있다.
 * GPIO를 하나씩 3초 동안 HIGH로 만들고 시리얼 모니터에 번호를 출력한다.
 * 불이 켜질 때 출력된 번호를 traffic_light.ino 의 RED_PIN / GREEN_PIN 에 넣으면 된다.
 *
 * - 처음(모든 핀 LOW)부터 불이 켜져 있고 특정 핀에서 꺼지면 LOW에서 켜지는 회로
 *   -> traffic_light.ino 의 ACTIVE_HIGH 를 false 로 설정
 * - Tools > USB CDC On Boot : Enabled, 시리얼 모니터 115200 baud
 */

// 기존 회로에서 트랜지스터 쪽으로 배선이 확인된 핀만 시험한다
// (다른 핀까지 찾으려면 {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 21} 로 바꾼다. 18, 19는 USB라 제외)
const int PINS[] = {3, 4, 5, 6};
const int PIN_COUNT = sizeof(PINS) / sizeof(PINS[0]);
const uint32_t ON_MS = 5000;
const uint32_t GAP_MS = 1000;

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < PIN_COUNT; i++) {
    pinMode(PINS[i], OUTPUT);
    digitalWrite(PINS[i], LOW);
  }
  delay(3000);                                 // 시리얼 모니터 열 시간
  Serial.println("핀 찾기 시작: 불이 켜질 때 표시되는 GPIO 번호를 적어두세요.");
}

void loop() {
  Serial.println(">> 모든 핀 LOW: 지금 켜져 있는 불이 있으면 적어두세요");
  delay(ON_MS);
  for (int i = 0; i < PIN_COUNT; i++) {
    Serial.printf(">> GPIO %d HIGH (%d/%d)\n", PINS[i], i + 1, PIN_COUNT);
    digitalWrite(PINS[i], HIGH);
    delay(ON_MS);
    digitalWrite(PINS[i], LOW);
    delay(GAP_MS);
  }
  Serial.println("---- 한 바퀴 끝, 다시 시작 ----");
}
