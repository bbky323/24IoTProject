#include <Arduino_JSON.h>
#include <AWS_IOT.h>  
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <MQUnifiedsensor.h> // MQ-135 라이브러리 

AWS_IOT testButton; 
//const char* ssid = "iPhone"; 
//const char* password = "jiwon1102";
const char* ssid = "KAU-Guest"; 
const char* password = "";
char HOST_ADDRESS[]="a3s0cos9xe395k-ats.iot.ap-northeast-2.amazonaws.com"; 
char CLIENT_ID[]= "YoonEsp32";
char sTOPIC_NAME[]= "$aws/things/ESP32_ProjectInput/shadow/update/delta"; // subscribe topic name
char pTOPIC_NAME[]= "$aws/things/ESP32_ProjectInput/shadow/update"; // publish topic name


int status = WL_IDLE_STATUS;
int msgCount=0,msgReceived = 0;
char payload[512];
char rcvdPayload[512];
const int buttonPin = 15; 
unsigned long preMil = 0;
const long intMil = 5000; // 버튼 바운싱 이슈 때문에 5초간은 안 눌리게 설정

// MQ135 Definitions
#define placa "ESP32"  // 보드명을 ESP32로 설정
#define Voltage_Resolution 3.3  // ESP32의 전압 분해능
#define pin 32  // ESP32의 D32 핀 사용
#define type "MQ-135"  // 센서 타입
#define ADC_Bit_Resolution 12  // ESP32는 12비트 ADC를 지원
#define RatioMQ135CleanAir 3.6  // RS / R0 = 3.6 ppm  
MQUnifiedsensor MQ135(placa, Voltage_Resolution, ADC_Bit_Resolution, pin, type);
// flag 무한 반복 막기
//int prevFlag = -1;
// 조도센서
const int cdsPin = 35; 
int lux = 0;

// BME 280 - 온습도
Adafruit_BME280 bme; 
int delayTime;

// 플래그 및 상태
int flag = 0;  // 앱 또는 자동 제어
float ctemp = 20.0, chumid = 30.0;
int buttonPressCount = 0;  // 버튼 누름 횟수
float temp = 0, humid = 0, co2 = 0;
char led[4] = "OFF";
char heat[4] = "OFF";
char fan[4] = "OFF";
char pump[4] = "OFF";

// AWS에 디바이스 상태와 센서 데이터를 업데이트하는 함수
// 값이 -1이라면 센서값이 읽히지 않았다는 뜻 
void publishToAWS(float temp = -1, float humid = -1, int lux = -1, float co2 = -1) {
    char updatePayload[512]; // Adjust buffer size if necessary

    // Construct JSON payload for the reported state
    sprintf(updatePayload,
            "{\"state\":{\"reported\":{\"temp\":%.2f,\"humid\":%.2f,\"lux\":%d,\"co2\":%.2f,\"led\":\"%s\",\"heat\":\"%s\",\"fan\":\"%s\",\"pump\":\"%s\", \"flag\":%d,\"ctemp\":%.2f,\"chumid\":%.2f }}}",
            temp == -1 ? 0 : temp,
            humid == -1 ? 0 : humid,
            lux == -1 ? 0 : lux,
            co2 == -1 ? 0 : co2,
            led, heat, fan, pump, flag, ctemp, chumid);

    // Publish the updated state to the reported topic
    if (testButton.publish(pTOPIC_NAME, updatePayload) == 0) { // Publish successfully
        Serial.print("Published updated state on AWS: ");
        Serial.println(updatePayload);
    } else { // Publish failed
        Serial.println("Failed to publish updated state on AWS");
    }
}

void mySubCallBackHandler(char *topicName, int payloadLen, char *payLoad) {
  strncpy(rcvdPayload, payLoad, payloadLen);
  rcvdPayload[payloadLen] = 0;
  msgReceived = 1; // flag 설정
  Serial.print("msgReceived: ");
  Serial.println(msgReceived);
  Serial.println("I'm callbackFunction");

  // JSON 파싱 및 flag 업데이트
  JSONVar receivedData = JSON.parse(rcvdPayload);
  if (JSON.typeof(receivedData) == "undefined") {
    Serial.println("Parsing input failed!");
    return;
  }

  bool stateChanged = false;
  if (receivedData.hasOwnProperty("state")) {
    JSONVar state = receivedData["state"];

    if (state.hasOwnProperty("flag")) {
      int newFlag = int(state["flag"]);
      if (newFlag != flag) {
        flag = newFlag;
        stateChanged = true;
      }
    }
    if (state.hasOwnProperty("led")) {
      const char *newLed = (const char *)state["led"];
      if (strcmp(newLed, led) != 0) {
        strncpy(led, newLed, sizeof(led) - 1);
        led[sizeof(led) - 1] = '\0'; // Ensure null termination
        stateChanged = true;
      }
    }
    if (state.hasOwnProperty("heat")) {
      const char *newHeat = (const char *)state["heat"];
      if (strcmp(newHeat, heat) != 0) {
        strncpy(heat, newHeat, sizeof(heat) - 1);
        heat[sizeof(heat) - 1] = '\0'; // Ensure null termination
        stateChanged = true;
      }
    }
    if (state.hasOwnProperty("fan")) {
      const char *newFan = (const char *)state["fan"];
      if (strcmp(newFan, fan) != 0) {
        strncpy(fan, newFan, sizeof(fan) - 1);
        fan[sizeof(fan) - 1] = '\0'; // Ensure null termination
        stateChanged = true;
      }
    }
    if (state.hasOwnProperty("pump")) {
      const char *newPump = (const char *)state["pump"];
      if (strcmp(newPump, pump) != 0) {
        strncpy(pump, newPump, sizeof(pump) - 1);
        pump[sizeof(pump) - 1] = '\0'; // Ensure null termination
        stateChanged = true;
      }
    }
    if (state.hasOwnProperty("ctemp")) {
      int newctemp = int(state["ctemp"]);
      if (newctemp != flag) {
        ctemp = newctemp;
        stateChanged = true;
      }
    }
    if (state.hasOwnProperty("chumid")) {
      int newchumid = int(state["chumid"]);
      if (newchumid != chumid) {
        chumid = newchumid;
        stateChanged = true;
      }
    }
  }

  // 변경 사항이 있는 경우에만 상태 업데이트를 게시
  if (stateChanged) {
     publishToAWS();
  }
}

// wifi 설정
void setupWiFi() {
  Serial.print("WIFI status = ");
  Serial.println(WiFi.getMode());
  WiFi.disconnect(true);
  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  delay(1000);
   
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi!");
  } else {
    Serial.println("Failed to connect to WiFi. Check credentials.");
    while (1); 
  }
}

//aws 설정
void setupAWS() {
  if(testButton.connect(HOST_ADDRESS,CLIENT_ID)== 0) {
    Serial.println("Connected to AWS");
    delay(1000);
    if(0==testButton.subscribe(sTOPIC_NAME,mySubCallBackHandler)) { 
      Serial.print("Subscribe Successfull:");
      Serial.println(sTOPIC_NAME);
    }
    else {
      Serial.println("Subscribe Failed, Check the Thing Name and Certificates");
      while(1);
    }
  }
  else {
    Serial.println("AWS connection failed, Check the HOST Address");
    while(1);
  }
}

// bme280 설정
void setupBME280() {
  Serial.println(F("Initializing BME280 sensor..."));
  if (!bme.begin(0x76)) {
    Serial.println("BME280 initialization failed!");
    while (1); 
  }
  Serial.println("BME280 Initialized!");
}

//mq135설정
void setupMQ135() {
  Serial.println("Initializing MQ135 sensor...");
  MQ135.init();
  float calcR0 = 0;
  for (int i = 0; i < 10; i++) {
    MQ135.update();
    calcR0 += MQ135.calibrate(RatioMQ135CleanAir);
    delay(500);
  }
  MQ135.setR0(calcR0 / 10);
  
  if (calcR0 == 0 || isinf(calcR0)) {
    Serial.println("MQ135 calibration error!");
    while (1); // Halt execution if calibration fails
  }
  Serial.println("MQ135 Initialized!");
}


void printBME280(float temp, float humid) {
  Serial.print(", Temperature = "); Serial.print(temp); Serial.print(" *C,");
  Serial.print(" Humidity = ");Serial.print(humid); Serial.print(" %,");
} 

void printMq135(float co2){
  Serial.print(" co2:  "); Serial.print(co2 + 400);
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT);
  setupWiFi();
  // WiFi 연결 후에만 AWS, BME280, MQ135 초기화
  setupAWS();
  setupBME280();
  setupMQ135();
}

void loop() {
  static unsigned long lastPublishTime = 0;
  unsigned long currentMillis = millis();

  // 버튼 상태 확인
  if (digitalRead(buttonPin) == HIGH && (millis() - preMil > intMil)) {
    preMil = millis();
    buttonPressCount++;
    Serial.print("Button Press Count: ");
    Serial.println(buttonPressCount);

//    // 버튼 누름에 따른 값 변경 및 유지
//    if (buttonPressCount == 1) {
//      temp = 50.0;
//      Serial.print("temp:");
//      Serial.println(temp);
//    } else if (buttonPressCount == 2) {
//      humid = 100.0;
//      Serial.print("humid:");
//      Serial.println(humid);
//    } else if (buttonPressCount == 3) {
//      co2 = 1000.0;
//      Serial.print("co2:");
//      Serial.println(co2);
//    } else if (buttonPressCount == 4) { // 모두 초기화
//      lux = 2000;
//      Serial.print("lux:");
//      Serial.println(lux);
//    } else if(buttonPressCount == 5){
//      temp = 0;
//      humid = 0;
//      co2 = 0;
//      buttonPressCount = 0;
//      Serial.println("Reinitialized!");
//      }
        if(buttonPressCount == 1){
          co2 = 2000.0;
          Serial.print("co2:");
          Serial.println(co2);
        }else if(buttonPressCount == 2){
          humid = 100.0;
          Serial.print("humid:");
          Serial.println(humid);
        }
        else if(buttonPressCount == 3){
        co2=0;
        humid=0;  
        buttonPressCount = 0;
        }
  }

    if (currentMillis - lastPublishTime >= 5000) {
      lastPublishTime = currentMillis;

      // 센서 값 읽기 및 업데이트
      // 버튼이 눌린 경우 해당 값 유지, 그렇지 않은 경우 센서 값 읽기
      if (buttonPressCount < 1 || buttonPressCount > 2) {
        temp = bme.readTemperature(); // 온도 값을 센서에서 읽어와 갱신
        humid = bme.readHumidity(); // 습도 값을 센서에서 읽어와 갱신
        MQ135.update();
        MQ135.setA(110.47);
        MQ135.setB(-2.862);
        co2 = MQ135.readSensor() + 400; // CO2 값을 센서에서 읽어와 갱신
        lux = analogRead(cdsPin); // 조도 센서 값 읽기
      }
      

      Serial.print("lux = ");
      Serial.print(lux);
      printBME280(temp, humid);
      printMq135(co2);

      // AWS에 데이터 전송
      publishToAWS(temp, humid, lux, co2);
      delay(1000);
    }

  // AWS로부터 수신된 메시지 처리
  if (msgReceived == 1) {
    msgReceived = 0;
    Serial.print("Received Message: ");
    Serial.println(rcvdPayload);
  }
}
