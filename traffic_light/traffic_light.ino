/*
 * ESP32-C3 횡단보도 신호등 제어
 *
 * 기능 1) 자동 모드 (전원 ON 시 기본)
 *   빨간불 5초 ON -> 초록불 5초 ON -> 무한 반복
 *
 * 기능 2) 수동 모드
 *   USB 시리얼 모니터, Wi-Fi 터미널(포트 23), 웹 페이지(포트 80) 중 아무 곳에서나 명령 입력
 *     R 10   : 빨간불 10초 ON (초록불은 자동 OFF)
 *     G 7    : 초록불 7초 ON  (빨간불은 자동 OFF)
 *     R      : 시간 없이 입력하면 계속 ON
 *     OFF    : 모두 OFF
 *     AUTO   : 자동 모드로 복귀
 *     STATUS : 현재 상태 표시
 *     HELP   : 명령어 목록
 *
 * 화살표 PCB: 남은 시간 비율만큼 화살표가 켜지고 하나씩 줄어든다. 마지막 1초는 점멸.
 *
 * 상태 표시 (1초마다)
 *   [수동] -----------------------------
 *   ▶ 빨간불  ON   남은 7초 / 유지 10초
 *     초록불  OFF
 *
 * 보드: ESP32C3 Dev Module (Arduino IDE)
 *   Tools > USB CDC On Boot : Enabled  (USB 시리얼 모니터 사용 시 필수)
 */

#include <WiFi.h>
#include <WebServer.h>

// ---------------- 사용자 설정 ----------------
// 공유기에 연결하려면 SSID/비밀번호 입력. 그대로 두거나 연결 실패 시 ESP32가 직접 AP를 만든다.
const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASS = "YOUR_PASSWORD";
const char* AP_SSID   = "TrafficLight-C3";
const char* AP_PASS   = "12345678";            // 8자 이상

const int  RED_PIN     = 4;                    // 빨간불 제어 핀 (트랜지스터/릴레이 입력)
const int  GREEN_PIN   = 5;                    // 초록불 제어 핀
const bool ACTIVE_HIGH = true;                 // 릴레이 모듈이 LOW에서 동작하면 false

// 화살표 PCB: 남은 시간을 화살표 개수로 표시 (pin_finder 로 찾은 번호 입력)
// ARROW_PINS[0] 이 마지막까지 남는 화살표. 사용하지 않는 칸은 -1.
// 화살표 전체가 선 1개로 묶여 있으면 첫 칸에만 번호를 넣으면 된다.
const int  ARROW_PINS[]      = {-1, -1, -1, -1, -1};
const bool ARROW_ACTIVE_HIGH = true;
const uint32_t ARROW_BLINK_MS = 1000;          // 남은 시간이 이 값 이하이면 화살표 점멸

const uint32_t AUTO_RED_MS   = 5000;
const uint32_t AUTO_GREEN_MS = 5000;
const uint32_t WIFI_TIMEOUT_MS = 15000;        // 공유기 연결 대기 후 AP 모드로 전환
// ---------------------------------------------

enum Light { LIGHT_OFF, LIGHT_RED, LIGHT_GREEN };
enum Mode  { MODE_AUTO, MODE_MANUAL };
enum Net   { NET_CONNECTING, NET_STA, NET_AP };

Mode     mode = MODE_AUTO;
Light    light = LIGHT_OFF;
uint32_t phaseStart = 0;
uint32_t phaseDuration = 0;                    // 0 = 시간 제한 없음
uint32_t lastReport = 0;

const int ARROW_COUNT = sizeof(ARROW_PINS) / sizeof(ARROW_PINS[0]);

Net      net = NET_CONNECTING;
uint32_t netStart = 0;

WebServer  web(80);
WiFiServer telnet(23);
WiFiClient telnetClient;

String serialLine;
String telnetLine;
int    telnetSkip = 0;                         // 텔넷 협상(IAC) 바이트 건너뛰기

// ---------------- 출력 ----------------
void out(const String& s) {
  Serial.println(s);
  if (telnetClient && telnetClient.connected()) {
    telnetClient.print(s);
    telnetClient.print("\r\n");
  }
}

// ---------------- 신호등 ----------------
void writePin(int pin, bool on) {
  digitalWrite(pin, (on == ACTIVE_HIGH) ? HIGH : LOW);
}

uint32_t remainingMs() {
  if (phaseDuration == 0) return 0;
  uint32_t elapsed = millis() - phaseStart;
  return elapsed >= phaseDuration ? 0 : phaseDuration - elapsed;
}

// 남은 시간 비율만큼 켜질 화살표 개수 (마지막 구간에서는 점멸)
int arrowsLit() {
  if (light == LIGHT_OFF) return 0;
  if (phaseDuration == 0) return ARROW_COUNT;
  uint32_t remain = remainingMs();
  if (remain <= ARROW_BLINK_MS && (millis() / 250) % 2) return 0;
  return (int)(((uint64_t)remain * ARROW_COUNT + phaseDuration - 1) / phaseDuration);
}

void updateArrows() {
  int lit = arrowsLit();
  for (int i = 0; i < ARROW_COUNT; i++) {
    if (ARROW_PINS[i] < 0) continue;
    bool on = i < lit;
    digitalWrite(ARROW_PINS[i], (on == ARROW_ACTIVE_HIGH) ? HIGH : LOW);
  }
}

String arrowBar() {
  int lit = arrowsLit();
  String s;
  for (int i = ARROW_COUNT - 1; i >= 0; i--) s += i < lit ? "▶" : "▷";
  return s;
}

String lightLine(Light which, const char* name) {
  bool on = (light == which);
  String s = on ? "▶ " : "  ";
  s += name;
  s += on ? "  ON " : "  OFF";
  if (on) {
    if (phaseDuration == 0) {
      s += "  (계속 유지)";
    } else {
      s += "  남은 " + String((remainingMs() + 999) / 1000) + "초 / 유지 " +
           String(phaseDuration / 1000.0, phaseDuration % 1000 ? 1 : 0) + "초";
    }
  }
  return s;
}

void printStatus() {
  out(String(mode == MODE_AUTO ? "[자동]" : "[수동]") + " -----------------------------");
  out(lightLine(LIGHT_RED, "빨간불"));
  out(lightLine(LIGHT_GREEN, "초록불"));
  out("  화살표  " + arrowBar());
  lastReport = millis();
}

void setLight(Light l, uint32_t durationMs) {
  // 먼저 둘 다 끈 뒤 필요한 쪽만 켜서 빨강/초록이 동시에 켜지는 순간이 없게 한다
  writePin(RED_PIN, false);
  writePin(GREEN_PIN, false);
  if (l == LIGHT_RED)   writePin(RED_PIN, true);
  if (l == LIGHT_GREEN) writePin(GREEN_PIN, true);

  light = l;
  phaseStart = millis();
  phaseDuration = durationMs;
  updateArrows();
  printStatus();
}

void updateLights() {
  if (phaseDuration == 0 || remainingMs() > 0) return;

  if (mode == MODE_AUTO) {
    if (light == LIGHT_RED) setLight(LIGHT_GREEN, AUTO_GREEN_MS);
    else                    setLight(LIGHT_RED, AUTO_RED_MS);
  } else {
    out("유지시간 종료 -> 모두 OFF  (AUTO 입력 시 자동 모드 복귀)");
    setLight(LIGHT_OFF, 0);
  }
}

// ---------------- 명령 처리 ----------------
String helpText() {
  return "명령어: R [초] 빨간불 | G [초] 초록불 | OFF | AUTO | STATUS | HELP\n"
         "예) R 10  -> 빨간불 10초,  G 7 -> 초록불 7초,  시간 생략 시 계속 유지";
}

String handleCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();
  if (cmd.length() == 0) return "";

  // 명령어와 숫자 분리 ("R 10", "R10", "RED 10" 모두 허용)
  int i = 0;
  while (i < (int)cmd.length() && isAlpha(cmd[i])) i++;
  String word = cmd.substring(0, i);
  String arg = cmd.substring(i);
  arg.trim();

  String msg;
  if (word == "R" || word == "RED" || word == "G" || word == "GREEN") {
    float sec = 0;
    if (arg.length() > 0) {
      sec = arg.toFloat();
      if (sec <= 0 || sec > 86400) {
        msg = "시간은 0보다 크고 86400초 이하로 입력하세요. 예) R 10";
        out(msg);
        return msg;
      }
    }
    bool red = (word[0] == 'R');
    mode = MODE_MANUAL;
    msg = String(red ? "빨간불" : "초록불") + " ON" +
          (sec > 0 ? ", 유지 " + arg + "초" : ", 계속 유지");
    out(">> " + msg);
    setLight(red ? LIGHT_RED : LIGHT_GREEN, (uint32_t)(sec * 1000));
  } else if (word == "OFF" || word == "STOP") {
    mode = MODE_MANUAL;
    msg = "모두 OFF";
    out(">> " + msg);
    setLight(LIGHT_OFF, 0);
  } else if (word == "AUTO" || word == "A") {
    mode = MODE_AUTO;
    msg = "자동 모드 시작";
    out(">> " + msg);
    setLight(LIGHT_RED, AUTO_RED_MS);
  } else if (word == "STATUS" || word == "S") {
    msg = "상태 표시";
    printStatus();
  } else if (word == "HELP" || word == "H" || cmd == "?") {
    msg = helpText();
    out(msg);
  } else {
    msg = "알 수 없는 명령: " + cmd;
    out(msg);
    out(helpText());
  }
  return msg;
}

// 한 바이트씩 받아 줄 단위로 명령 실행
void feed(String& buf, char c) {
  if (c == '\r' || c == '\n') {
    if (buf.length() > 0) handleCommand(buf);
    buf = "";
  } else if (buf.length() < 64) {
    buf += c;
  }
}

void readSerial() {
  while (Serial.available()) feed(serialLine, (char)Serial.read());
}

void readTelnet() {
  if (net == NET_CONNECTING) return;

  if (telnet.hasClient()) {
    if (telnetClient) telnetClient.stop();     // 한 번에 한 명만
    telnetClient = telnet.accept();
    telnetLine = "";
    telnetSkip = 0;
    out("Wi-Fi 터미널 연결됨");
    out(helpText());
    printStatus();
  }

  while (telnetClient && telnetClient.available()) {
    uint8_t b = telnetClient.read();
    if (telnetSkip > 0) { telnetSkip--; continue; }
    if (b == 0xFF) { telnetSkip = 2; continue; }
    if (b < 0x80) feed(telnetLine, (char)b);
  }
}

// ---------------- 웹 페이지 ----------------
const char PAGE[] PROGMEM = R"HTML(<!doctype html>
<html lang="ko"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>신호등 제어</title>
<style>
body{font-family:sans-serif;background:#222;color:#eee;max-width:420px;margin:20px auto;padding:0 16px}
.row{display:flex;align-items:center;gap:12px;margin:10px 0;font-size:20px}
.arrow{width:28px;font-size:26px;color:#ffd400;visibility:hidden}
.lamp{width:56px;height:56px;border-radius:50%;background:#333;border:3px solid #555}
.red.on{background:#ff2a2a;box-shadow:0 0 20px #ff2a2a}
.green.on{background:#22e05a;box-shadow:0 0 20px #22e05a}
.time{font-size:16px;color:#bbb}
#bar{font-size:30px;letter-spacing:4px;color:#555}
#bar.red{color:#ff2a2a}#bar.green{color:#22e05a}
input,button{font-size:16px;padding:8px;margin:4px 0}
input{width:80px}
button{border:0;border-radius:6px;cursor:pointer}
#mode{font-weight:bold}
</style></head><body>
<h2>횡단보도 신호등</h2>
<div>모드: <span id="mode">-</span></div>
<div class="row"><span class="arrow" id="aR">▶</span><div class="lamp red" id="lR"></div><div>빨간불<br><span class="time" id="tR">OFF</span></div></div>
<div class="row"><span class="arrow" id="aG">▶</span><div class="lamp green" id="lG"></div><div>초록불<br><span class="time" id="tG">OFF</span></div></div>
<div>남은 시간 화살표: <span id="bar">▷▷▷▷▷</span></div>
<hr>
<div>유지 시간(초, 비우면 계속): <input id="sec" type="number" min="0.1" step="0.1" value="10"></div>
<button style="background:#ff2a2a;color:#fff" onclick="send('R')">빨간불 ON</button>
<button style="background:#22e05a" onclick="send('G')">초록불 ON</button>
<button onclick="cmd('OFF')">모두 OFF</button>
<button onclick="cmd('AUTO')">자동 모드</button>
<p id="msg" class="time"></p>
<script>
function cmd(c){fetch('/cmd?c='+encodeURIComponent(c)).then(r=>r.text()).then(t=>msg.textContent=t);}
function send(l){cmd(l+' '+sec.value);}
function show(k,on,d){
  document.getElementById('a'+k).style.visibility=on?'visible':'hidden';
  document.getElementById('l'+k).classList.toggle('on',on);
  document.getElementById('t'+k).textContent=!on?'OFF':(d.duration==0?'ON (계속 유지)':'ON  남은 '+Math.ceil(d.remain)+'초 / 유지 '+d.duration+'초');
}
function poll(){fetch('/status').then(r=>r.json()).then(d=>{
  mode.textContent=d.mode=='AUTO'?'자동':'수동';
  show('R',d.light=='RED',d);show('G',d.light=='GREEN',d);
  let b='';for(let i=d.arrowCount-1;i>=0;i--)b+=i<d.arrows?'▶':'▷';
  bar.textContent=b;bar.className=d.light=='RED'?'red':d.light=='GREEN'?'green':'';
}).catch(()=>{}).finally(()=>setTimeout(poll,300));}
poll();
</script></body></html>)HTML";

void startServers() {
  web.on("/", []() { web.send_P(200, "text/html; charset=utf-8", PAGE); });
  web.on("/cmd", []() { web.send(200, "text/plain; charset=utf-8", handleCommand(web.arg("c"))); });
  web.on("/status", []() {
    const char* l = light == LIGHT_RED ? "RED" : light == LIGHT_GREEN ? "GREEN" : "OFF";
    String json = String("{\"mode\":\"") + (mode == MODE_AUTO ? "AUTO" : "MANUAL") +
                  "\",\"light\":\"" + l +
                  "\",\"remain\":" + String(remainingMs() / 1000.0, 1) +
                  ",\"duration\":" + String(phaseDuration / 1000.0, 1) +
                  ",\"arrows\":" + String(arrowsLit()) +
                  ",\"arrowCount\":" + String(ARROW_COUNT) + "}";
    web.send(200, "application/json", json);
  });
  web.begin();
  telnet.begin();
  telnet.setNoDelay(true);
}

// ---------------- Wi-Fi (부팅을 막지 않도록 loop에서 진행) ----------------
void startAP() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  net = NET_AP;
  startServers();
  out("Wi-Fi AP 모드: PC를 '" + String(AP_SSID) + "' (비밀번호 " + AP_PASS + ")에 연결");
  out("  웹: http://" + WiFi.softAPIP().toString() + "   터미널: " + WiFi.softAPIP().toString() + " 포트 23");
}

void netBegin() {
  if (String(WIFI_SSID) == "YOUR_SSID") {
    startAP();
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  netStart = millis();
  out("Wi-Fi '" + String(WIFI_SSID) + "' 연결 중...");
}

void netUpdate() {
  if (net != NET_CONNECTING) return;
  if (WiFi.status() == WL_CONNECTED) {
    net = NET_STA;
    startServers();
    out("Wi-Fi 연결됨");
    out("  웹: http://" + WiFi.localIP().toString() + "   터미널: " + WiFi.localIP().toString() + " 포트 23");
  } else if (millis() - netStart > WIFI_TIMEOUT_MS) {
    out("공유기 연결 실패 -> AP 모드로 전환");
    startAP();
  }
}

// ---------------- 메인 ----------------
void setup() {
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  writePin(RED_PIN, false);
  writePin(GREEN_PIN, false);
  for (int i = 0; i < ARROW_COUNT; i++) {
    if (ARROW_PINS[i] >= 0) pinMode(ARROW_PINS[i], OUTPUT);
  }

  Serial.begin(115200);

  // 기능 1: 전원 ON 즉시 자동 모드 시작 (빨간불부터)
  mode = MODE_AUTO;
  setLight(LIGHT_RED, AUTO_RED_MS);

  netBegin();
  out(helpText());
}

void loop() {
  readSerial();
  netUpdate();
  if (net != NET_CONNECTING) {
    web.handleClient();
    readTelnet();
  }

  updateLights();
  updateArrows();

  if (millis() - lastReport >= 1000) printStatus();
}
