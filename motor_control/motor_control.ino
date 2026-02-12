/*
 * MOBIIXE - DC 모터 제어 프로그램
 *
 * L298N 모터 드라이버를 사용하여 DC 모터 2개를 제어하는 코드
 * 기능: 전진, 후진, 좌회전, 우회전, 정지, 속도 조절
 */

// === 모터 A (왼쪽) 핀 설정 ===
const int MOTOR_A_EN  = 5;   // 모터A 속도 제어 (PWM)
const int MOTOR_A_IN1 = 7;   // 모터A 방향 제어 핀1
const int MOTOR_A_IN2 = 8;   // 모터A 방향 제어 핀2

// === 모터 B (오른쪽) 핀 설정 ===
const int MOTOR_B_EN  = 6;   // 모터B 속도 제어 (PWM)
const int MOTOR_B_IN3 = 9;   // 모터B 방향 제어 핀3
const int MOTOR_B_IN4 = 10;  // 모터B 방향 제어 핀4

// === 기본 속도 설정 (0~255) ===
int motorSpeed = 150;

void setup() {
  // 시리얼 통신 시작 (디버깅 및 명령 입력용)
  Serial.begin(9600);

  // 모터 핀을 출력으로 설정
  pinMode(MOTOR_A_EN, OUTPUT);
  pinMode(MOTOR_A_IN1, OUTPUT);
  pinMode(MOTOR_A_IN2, OUTPUT);
  pinMode(MOTOR_B_EN, OUTPUT);
  pinMode(MOTOR_B_IN3, OUTPUT);
  pinMode(MOTOR_B_IN4, OUTPUT);

  // 초기 상태: 정지
  stop();

  Serial.println("=== MOBIIXE 모터 제어 시스템 ===");
  Serial.println("명령어 안내:");
  Serial.println("  W - 전진");
  Serial.println("  S - 후진");
  Serial.println("  A - 좌회전");
  Serial.println("  D - 우회전");
  Serial.println("  X - 정지");
  Serial.println("  1~9 - 속도 조절 (1=느림, 9=빠름)");
}

void loop() {
  // 시리얼로 명령을 수신하여 모터 제어
  if (Serial.available() > 0) {
    char cmd = Serial.read();

    switch (cmd) {
      case 'W':
      case 'w':
        forward();
        Serial.println(">> 전진");
        break;

      case 'S':
      case 's':
        backward();
        Serial.println(">> 후진");
        break;

      case 'A':
      case 'a':
        turnLeft();
        Serial.println(">> 좌회전");
        break;

      case 'D':
      case 'd':
        turnRight();
        Serial.println(">> 우회전");
        break;

      case 'X':
      case 'x':
        stop();
        Serial.println(">> 정지");
        break;

      default:
        // 숫자 1~9 입력 시 속도 변경
        if (cmd >= '1' && cmd <= '9') {
          motorSpeed = map(cmd - '0', 1, 9, 80, 255);
          setSpeed(motorSpeed);
          Serial.print(">> 속도 변경: ");
          Serial.println(motorSpeed);
        }
        break;
    }
  }
}

// 전진: 양쪽 모터 정방향 회전
void forward() {
  digitalWrite(MOTOR_A_IN1, HIGH);
  digitalWrite(MOTOR_A_IN2, LOW);
  digitalWrite(MOTOR_B_IN3, HIGH);
  digitalWrite(MOTOR_B_IN4, LOW);
  setSpeed(motorSpeed);
}

// 후진: 양쪽 모터 역방향 회전
void backward() {
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, HIGH);
  digitalWrite(MOTOR_B_IN3, LOW);
  digitalWrite(MOTOR_B_IN4, HIGH);
  setSpeed(motorSpeed);
}

// 좌회전: 왼쪽 모터 정지, 오른쪽 모터 정방향
void turnLeft() {
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, LOW);
  digitalWrite(MOTOR_B_IN3, HIGH);
  digitalWrite(MOTOR_B_IN4, LOW);
  analogWrite(MOTOR_A_EN, 0);
  analogWrite(MOTOR_B_EN, motorSpeed);
}

// 우회전: 왼쪽 모터 정방향, 오른쪽 모터 정지
void turnRight() {
  digitalWrite(MOTOR_A_IN1, HIGH);
  digitalWrite(MOTOR_A_IN2, LOW);
  digitalWrite(MOTOR_B_IN3, LOW);
  digitalWrite(MOTOR_B_IN4, LOW);
  analogWrite(MOTOR_A_EN, motorSpeed);
  analogWrite(MOTOR_B_EN, 0);
}

// 정지: 양쪽 모터 모두 멈춤
void stop() {
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, LOW);
  digitalWrite(MOTOR_B_IN3, LOW);
  digitalWrite(MOTOR_B_IN4, LOW);
  setSpeed(0);
}

// 속도 설정: 양쪽 모터에 PWM 값 적용
void setSpeed(int speed) {
  analogWrite(MOTOR_A_EN, speed);
  analogWrite(MOTOR_B_EN, speed);
}
