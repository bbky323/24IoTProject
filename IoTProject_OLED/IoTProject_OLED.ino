#include <WiFi.h>
#include <AWS_IOT.h>
#include <time.h>
#include <Arduino_JSON.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>

/* If using software SPI (the default case): */
#define OLED_MOSI 13 // 9
#define OLED_CLK 14 //10
#define OLED_DC 26 //11
#define OLED_CS 12 //12
#define OLED_RESET 27 //13
Adafruit_SH1106 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH 16
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };
#if (SH1106_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SH1106.h!");
#endif

AWS_IOT Display; // 변수 선언
const char* ssid = "GIYOUNG"; //Wi-Fi 연결
const char* password = "kk323625";
char HOST_ADDRESS[]="a3s0cos9xe395k-ats.iot.ap-northeast-2.amazonaws.com";
char CLIENT_ID[]= "BaeESP32";
char sTOPIC_NAME[]= "$aws/things/ESP32_ProjectDisplay/shadow/update/delta"; // subscribe topic name
char pTOPIC_NAME[]= "$aws/things/ESP32_ProjectDisplay/shadow/update"; // publish topic name

// AWS 관련 상태 변수
int msgReceived = 0;
char rcvdPayload[512]; // 수신된 메시지 버퍼
char payload[512]; // Publish 메시지 버퍼

float temp = 0.0;
float humid = 0.0;
int lux = 0;
float co2 = 0.0;
String ledState = "OFF";
String heatState = "OFF";
String fanState = "OFF";
String pumpState = "OFF";
unsigned long lastTimeUpdate = 0; // 마지막 시간 업데이트 기록 변수


// 콜백 핸들러 (Subscribe 메시지 처리)
void mySubCallbackHandler(char* topicName, int payloadLen, char* payload) {
  strncpy(rcvdPayload, payload, payloadLen);
  rcvdPayload[payloadLen] = 0;
  msgReceived = 1;
}


void setup() {
  Serial.begin(115200);
  //++choi This is here to force the ESP32 to reset the WiFi and initialize correctly.
  Serial.print("WIFI status = ");
  Serial.println(WiFi.getMode());
  WiFi.disconnect(true);
  delay(1000);
  WiFi.mode(WIFI_STA);
  delay(1000);
  Serial.print("WIFI status = ");
  Serial.println(WiFi.getMode()); //++choi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to wifi");

  // NTP 시간 설정
  setupNTP();

  // AWS IoT 연결
  if (Display.connect(HOST_ADDRESS, CLIENT_ID) == 0) {
    Serial.println("AWS IoT connected!");
    delay(1000);
    if (Display.subscribe(sTOPIC_NAME, mySubCallbackHandler) == 0) {
      Serial.println("Subscribed to topic successfully!");
    } else {
      Serial.println("Failed to subscribe to topic. Check the topic name.");
      while (1)
        delay(1000);
    }
  } else {
    Serial.println("AWS IoT connection failed. Check HOST_ADDRESS and CLIENT_ID.");
    while (1)
      delay(1000);
  }

  // OLED 초기화
  display.begin(SH1106_SWITCHCAPVCC);
  display.display();
  delay(2000);
  display.clearDisplay();

  Serial.println("Setup complete. Display test starting.");
}

void loop() {
  // AWS 메시지 수신 처리
  if (msgReceived == 1) {
    msgReceived = 0;
    Serial.print("Received Message: ");
    Serial.println(rcvdPayload);

    // JSON 메시지 파싱
    JSONVar myObj = JSON.parse(rcvdPayload);
    if (JSON.typeof(myObj) == "undefined") {
      Serial.println("Failed to parse JSON!");
      return;
    }

    JSONVar state = myObj["state"];

    // 각 필드 값 갱신 (NULL 처리) 및 상태 변화 감지
    if (state.hasOwnProperty("temp")) {
      float newTemp = (double)state["temp"];
      temp = newTemp;
    }
    if (state.hasOwnProperty("humid")) {
      float newHumid = (double)state["humid"];
      humid = newHumid;
    }
    if (state.hasOwnProperty("lux")) {
      int newLux = (int)state["lux"];
      lux = newLux;
    }
    if (state.hasOwnProperty("co2")) {
      float newCo2 = (double)state["co2"];
      co2 = newCo2;
    }
    if (state.hasOwnProperty("led")) {
      const char* newLedState = (const char*)state["led"];
      ledState = newLedState;
    }
    if (state.hasOwnProperty("heat")) {
      const char* newHeatState = (const char*)state["heat"];
      heatState = newHeatState;
    }
    if (state.hasOwnProperty("fan")) {
      const char* newFanState = (const char*)state["fan"];
      fanState = newFanState;
    }
    if (state.hasOwnProperty("pump")) {
      const char* newPumpState = (const char*)state["pump"];
      pumpState = newPumpState;
    }

    // OLED 디스플레이에 표시
    displayData(temp, humid, lux, co2, ledState.c_str(), heatState.c_str(), fanState.c_str(), pumpState.c_str());
  }

  // 시간 업데이트 로직 (1초마다 갱신)
  unsigned long currentMillis = millis();
  if (currentMillis - lastTimeUpdate >= 1000) { // 1초 경과 확인
    lastTimeUpdate = currentMillis;

    // OLED 디스플레이에 현재 시간 업데이트
    updateTimeDisplay();
  }


  delay(1000);
  
  char payload[512];

  // JSON 문자열 생성
  sprintf(payload,
          "{\"state\": {\"reported\": {\"temp\": %.2f, \"humid\": %.2f, \"lux\": %d, \"co2\": %.2f, \"led\": \"%s\", \"heat\": \"%s\", \"fan\": \"%s\", \"pump\": \"%s\"}}}",
          temp, humid, lux, co2, ledState.c_str(), heatState.c_str(), fanState.c_str(), pumpState.c_str());

  // Shadow Update Topic으로 Publish
  if (Display.publish(pTOPIC_NAME, payload) == 0) {
      Serial.print("Published Message: ");
      Serial.println(payload);
  } else {
      Serial.println("Failed to publish message.");
  }

}


// NTP 설정 함수
void setupNTP() {
  configTime(9 * 3600, 0, "pool.ntp.org", "time.nist.gov"); // UTC 시간 동기화
  Serial.print("Waiting for time sync...");
  while (time(nullptr) < 24 * 3600) { // 시간을 동기화할 때까지 대기
    delay(100);
    Serial.print(".");
  }
  Serial.println("\nTime synchronized!");
}

// OLED 디스플레이 업데이트
void displayData(float temp, float humid, int lux, int co2, const char* ledState, const char* heatState, const char* fanState, const char* pumpState) {
  display.clearDisplay();

   // 왼쪽 데이터 표시 (온도, 습도, 조도, CO2)
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0, 0);
  display.print("Temp: ");
  display.println(temp, 1);

  display.setCursor(0, 10);
  display.print("Humid: ");
  display.println(humid, 1);

  display.setCursor(0, 20);
  display.print("Lux: ");
  display.println(lux);

  display.setCursor(0, 30);
  display.print("CO2: ");
  display.println(co2);

  // 오른쪽 상태 표시 (LED, HEAT, FAN, PUMP)
  display.setCursor(69, 0);
  display.println("LED:");
  setStatusColor(100, 0, ledState);

  display.setCursor(69, 10);
  display.println("HEAT:");
  setStatusColor(100, 10, heatState);

  display.setCursor(69, 20);
  display.println("FAN:");
  setStatusColor(100, 20, fanState);

  display.setCursor(69, 30);
  display.println("PUMP:");
  setStatusColor(100, 30, pumpState);

}

// 상태에 따라 색상 설정 (ON: 반전, OFF: 기본)
void setStatusColor(int x, int y, const char* state) {
  if (strcmp(state, "ON") == 0) {
    display.setTextColor(BLACK, WHITE); // 반전된 텍스트
  } else {
    display.setTextColor(WHITE);       // 기본 텍스트
  }
  display.setCursor(x, y);
  display.println(state);
  display.setTextColor(WHITE);         // 기본 텍스트로 복구
}

void updateTimeDisplay() {
  // 하단에 현재 시간 표시
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);

  display.setCursor(0, 50);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.fillRect(0, 50, 128, 10, BLACK); // 이전 시간 영역 지우기
  display.printf("Time: %02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  display.display();

  // 디버그용 출력
  Serial.printf("Updated Time: %02d:%02d:%02d\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}
