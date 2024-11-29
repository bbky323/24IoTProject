#include <AWS_IOT.h>
#include <WiFi.h>
#include <Arduino_JSON.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
AWS_IOT testButton;

int status = WL_IDLE_STATUS;
int msgCount = 0, msgReceived = 0;
char payload[512];
char rcvdPayload[512];
unsigned long preMil = 0;
unsigned long preMilBtn = 0;
const long intMil = 5000;

// WiFi 및 AWS IoT 설정
const char* ssid = "MINSOO";
const char* password = "12250906";
char HOST_ADDRESS[] = "a3s0cos9xe395k-ats.iot.ap-northeast-2.amazonaws.com"; // AWS IoT 엔드포인트
char CLIENT_ID[] = "ESP32_ProjectOutput"; // 클라이언트 ID
char pTOPIC_NAME[] = "$aws/things/ESP32_ProjectOutput/shadow/update"; // Shadow 업데이트 토픽
char sTOPIC_NAME[] = "$aws/things/ESP32_ProjectOutput/shadow/update/delta"; // Shadow delta 토픽

#define GPIO_LED 32       // LED 제어 GPIO 핀
#define GPIO_HEAT 21     // 히터 제어 GPIO 핀
#define RELAY_FAN_PIN 19 // 팬 릴레이 핀
#define RELAY_PUMP_PIN 18 // 펌프 릴레이 핀

AWS_IOT awsIot;

// 현재 상태를 추적하기 위한 변수
String currentLedState = "OFF";
String currentHeatState = "OFF";
String currentFanState = "OFF";
String currentPumpState = "OFF";

void mySubCallBackHandler(char* topicName, int payloadLen, char* payLoad) {
    strncpy(rcvdPayload, payLoad, payloadLen);
    rcvdPayload[payloadLen] = 0;
    msgReceived = 1;
}

void setup() {
    Serial.begin(115200);
    WiFi.disconnect(true);
    delay(1000);
    WiFi.mode(WIFI_STA);
    delay(1000);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi..");
    }
    Serial.println("Connected to wifi");

    if (testButton.connect(HOST_ADDRESS, CLIENT_ID) == 0) {
        Serial.println("Connected to AWS");
        delay(1000);
        if (0 == testButton.subscribe(sTOPIC_NAME, mySubCallBackHandler)) {
            Serial.println("Subscribe Successful");
        } else {
            Serial.println("Subscribe Failed, Check the Thing Name and Certificates");
            while (1);
        }
    } else {
        Serial.println("AWS connection failed, Check the HOST Address");
        while (1);
    }

    pinMode(GPIO_LED, OUTPUT);
    pinMode(GPIO_HEAT, OUTPUT);
    pinMode(RELAY_FAN_PIN, OUTPUT);
    pinMode(RELAY_PUMP_PIN, OUTPUT);

    // 초기 상태 설정 (모두 OFF)
    digitalWrite(GPIO_LED, LOW);
    digitalWrite(GPIO_HEAT, LOW);
    digitalWrite(RELAY_FAN_PIN, LOW);
    digitalWrite(RELAY_PUMP_PIN, LOW);
    currentLedState = "OFF";
    currentHeatState = "OFF";
    currentFanState = "OFF";
    currentPumpState = "OFF";

    // 초기 상태 보고
    JSONVar reportedPayload;
    reportedPayload["state"]["reported"]["led"] = currentLedState;
    reportedPayload["state"]["reported"]["heat"] = currentHeatState;
    reportedPayload["state"]["reported"]["fan"] = currentFanState;
    reportedPayload["state"]["reported"]["pump"] = currentPumpState;

    String jsonString = JSON.stringify(reportedPayload);
    jsonString.replace(" ", ""); // 공백 제거
    char payloadBuffer[512];
    strncpy(payloadBuffer, jsonString.c_str(), sizeof(payloadBuffer) - 1);
    payloadBuffer[sizeof(payloadBuffer) - 1] = 0;

    Serial.print("Initial payload: ");
    Serial.println(payloadBuffer);

    if (awsIot.publish(pTOPIC_NAME, payloadBuffer) == 0) {
        Serial.println("Initial state successfully reported");
    } else {
        Serial.println("Failed to report initial state");
    }

    delay(2000);
}

unsigned long lastPrintTime = 0; // 마지막 상태 출력 시간
const long printInterval = 5000; // 상태 출력 주기 (5초)

void loop() {
    if (msgReceived == 1) {
        msgReceived = 0;

        Serial.print("Received payload: ");
        Serial.println(rcvdPayload);

        // Parse the received delta payload
        JSONVar deltaState = JSON.parse(rcvdPayload);
        if (JSON.typeof(deltaState) == "undefined") {
            Serial.println("Failed to parse delta message!");
            return;
        }

        if (deltaState.hasOwnProperty("state")) {
            JSONVar desiredState = deltaState["state"];
            JSONVar reportedState;

            // 상태 업데이트 로직
            bool stateChanged = false;

            if (desiredState.hasOwnProperty("led")) {
                String ledState = (const char*)desiredState["led"];
                if (ledState != currentLedState) {
                    currentLedState = ledState;
                    digitalWrite(GPIO_LED, ledState == "ON" ? HIGH : LOW);
                    Serial.println("LED turned " + ledState);
                    stateChanged = true;
                }
                reportedState["led"] = currentLedState;
            }

            if (desiredState.hasOwnProperty("heat")) {
                String heatState = (const char*)desiredState["heat"];
                if (heatState != currentHeatState) {
                    currentHeatState = heatState;
                    digitalWrite(GPIO_HEAT, heatState == "ON" ? HIGH : LOW);
                    Serial.println("Heat turned " + heatState);
                    stateChanged = true;
                }
                reportedState["heat"] = currentHeatState;
            }

            if (desiredState.hasOwnProperty("fan")) {
                String fanState = (const char*)desiredState["fan"];
                if (fanState != currentFanState) {
                    currentFanState = fanState;
                    digitalWrite(RELAY_FAN_PIN, fanState == "ON" ? HIGH : LOW);
                    Serial.println("Fan turned " + fanState);
                    stateChanged = true;
                }
                reportedState["fan"] = currentFanState;
            }

            if (desiredState.hasOwnProperty("pump")) {
                String pumpState = (const char*)desiredState["pump"];
                if (pumpState != currentPumpState) {
                    currentPumpState = pumpState;
                    digitalWrite(RELAY_PUMP_PIN, pumpState == "ON" ? HIGH : LOW);
                    Serial.println("Pump turned " + pumpState);
                    stateChanged = true;
                }
                reportedState["pump"] = currentPumpState;
            }

            if (stateChanged) {
                JSONVar reportedPayload;
                reportedPayload["state"]["reported"] = reportedState;

                String jsonString = JSON.stringify(reportedPayload);
                jsonString.replace(" ", ""); // 공백 제거
                char payloadBuffer[512];
                strncpy(payloadBuffer, jsonString.c_str(), sizeof(payloadBuffer) - 1);
                payloadBuffer[sizeof(payloadBuffer) - 1] = 0;

                Serial.print("Prepared payload: ");
                Serial.println(payloadBuffer);

                // 성공할 때까지 publish 재시도
                while (awsIot.publish(pTOPIC_NAME, payloadBuffer) != 0) {
                    Serial.println("Failed to update reported state. Retrying...");
                    delay(500); // 500ms 대기 후 다시 시도
                }
                Serial.println("Reported state successfully updated");
            } else {
                Serial.println("No changes to report.");
            }
        } else {
            Serial.println("State object not found in delta message");
        }
        delay(1000);
    }

    // 현재 상태를 일정 시간 간격으로 출력
    unsigned long currentMillis = millis();
    if (currentMillis - lastPrintTime >= printInterval) {
        lastPrintTime = currentMillis;

        Serial.println("Current States:");
        Serial.println("LED State: " + currentLedState);
        Serial.println("Heat State: " + currentHeatState);
        Serial.println("Fan State: " + currentFanState);
        Serial.println("Pump State: " + currentPumpState);
        Serial.println("----------------------------");
    }
}
