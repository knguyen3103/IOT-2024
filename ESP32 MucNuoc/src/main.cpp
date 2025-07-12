#define BLYNK_TEMPLATE_ID "TMPL63U3Ka_8E"
#define BLYNK_TEMPLATE_NAME "ESP32 MucNuoc"
#define BLYNK_AUTH_TOKEN "dGnGeCy-rQLRUspY3xYdapDF82PTWGi8"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Cấu hình WiFi
char ssid[] = "Wokwi-GUEST";
char pass[] = "";
bool wifiConnected = false; // Biến theo dõi trạng thái kết nối WiFi

// Cấu hình cảm biến và thiết bị
#define TRIG_PIN 5
#define ECHO_PIN 18
#define RELAY_PIN 4
#define BUZZER_PIN 23
#define LED_RED 15
#define LED_BLUE 2
#define BUTTON_PIN 0

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Biến toàn cục
bool autoMode = true;
float waterLevelPercentage = 0; // Biến lưu trữ mức nước (%)
float waterLevelCm = 0;         // Biến lưu trữ mức nước (cm)
bool relayState = LOW;           // Biến lưu trữ trạng thái relay

float getDistanceCm();
float calculateWaterLevelPercentage(float distanceCm);
float calculateWaterLevelCm(float distanceCm);
void displayOled();
void updateBlynk();

// Giả định khoảng cách tối đa từ cảm biến đến đáy bể (cm)
const float MAX_DISTANCE_CM = 25.0; // Điều chỉnh giá trị này theo thực tế
const int LOW_WATER_THRESHOLD = 30;   // Mức nước (%) coi là cạn
const int HIGH_WATER_THRESHOLD = 80;  // Mức nước (%) coi là đầy
const int SAFE_WATER_THRESHOLD = 40;   // Mức nước (%) dưới mức này sẽ cảnh báo buzzer
const int PUMP_ON_THRESHOLD = 30;     // Ngưỡng bật bơm (%)
const int PUMP_OFF_THRESHOLD = 80;    // Ngưỡng tắt bơm (%)

BLYNK_WRITE(V4) {
    autoMode = param.asInt(); // Đọc trạng thái công tắc Auto/Manual từ Blynk (1: Auto, 0: Manual)
    if (!autoMode) {
        digitalWrite(RELAY_PIN, relayState); // Cập nhật trạng thái relay khi chuyển sang Manual
    }
}

// Hàm nhận lệnh điều khiển Relay từ Blynk khi ở chế độ Manual (gán nút nhấn cho V5 trên Blynk)
BLYNK_WRITE(V5) {
    if (!autoMode) {
        relayState = param.asInt();
        digitalWrite(RELAY_PIN, relayState);
    }
}

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, pass);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected");
    wifiConnected = true;
    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

    // Cấu hình GPIO
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // Khởi tạo OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("OLED không khởi động được");
        while (true);
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Dang khoi tao...");
    display.display();
}

void loop() {
    if (wifiConnected) {
        Blynk.run();
    } else if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi reconnected");
        wifiConnected = true;
        Blynk.connect(); // Thử kết nối lại Blynk
    } else if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi mất kết nối, đang kết nối lại...");
        WiFi.reconnect();
        delay(5000); // Chờ một chút trước khi thử lại
    }

    float distanceCm = getDistanceCm();
    // Giới hạn giá trị khoảng cách sau khi đọc
    distanceCm = constrain(distanceCm, 2, MAX_DISTANCE_CM + 5);
    waterLevelPercentage = calculateWaterLevelPercentage(distanceCm);
    waterLevelCm = calculateWaterLevelCm(distanceCm);

    Serial.print("Khoảng cách: ");
    Serial.print(distanceCm);
    Serial.print(" cm, Mức nước: ");
    Serial.print(waterLevelPercentage);
    Serial.print("% (");
    Serial.print(waterLevelCm);
    Serial.println(" cm)");

    displayOled();
    updateBlynk();

    // Điều khiển LED cảnh báo trên ESP32 - Sửa đổi để tắt khi mức nước trung bình
    if (waterLevelPercentage >= LOW_WATER_THRESHOLD && waterLevelPercentage <= HIGH_WATER_THRESHOLD) {
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_BLUE, LOW);
    } else if (waterLevelPercentage < LOW_WATER_THRESHOLD) {
        digitalWrite(LED_RED, HIGH);
        digitalWrite(LED_BLUE, LOW);
    } else if (waterLevelPercentage > HIGH_WATER_THRESHOLD) {
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_BLUE, HIGH);
    }

    // Điều khiển Buzzer cảnh báo
    if (waterLevelPercentage < SAFE_WATER_THRESHOLD) {
        tone(BUZZER_PIN, 1000);
    } else {
        noTone(BUZZER_PIN);
    }

    // Chuyển đổi chế độ bằng nút nhấn vật lý (vẫn giữ lại)
    if (digitalRead(BUTTON_PIN) == LOW) {
        autoMode = !autoMode;
        Blynk.virtualWrite(V4, autoMode); // Cập nhật trạng thái công tắc Blynk
        if (!autoMode) {
            digitalWrite(RELAY_PIN, relayState); // Cập nhật trạng thái relay khi chuyển sang Manual
        }
        delay(500);
    }

    // Điều khiển Relay ở chế độ tự động
    if (autoMode) {
        if (waterLevelPercentage < PUMP_ON_THRESHOLD) {
            digitalWrite(RELAY_PIN, HIGH);
        } else if (waterLevelPercentage > PUMP_OFF_THRESHOLD) {
            digitalWrite(RELAY_PIN, LOW);
        }
    }

    delay(1000);
}

float getDistanceCm() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH);
    delay(50); // Thêm delay nhỏ sau khi đọc cảm biến
    return duration * 0.034 / 2;
}

float calculateWaterLevelPercentage(float distanceCm) {
    float level = map(distanceCm, 2, MAX_DISTANCE_CM, 100, 0);
    return constrain(level, 0, 100);
}

float calculateWaterLevelCm(float distanceCm) {
    float level = MAX_DISTANCE_CM - distanceCm;
    return constrain(level, 0, MAX_DISTANCE_CM);
}

void displayOled() {
    display.clearDisplay();
    display.setTextSize(2); // Increased text size for better visibility
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("Muc Nuoc: ");
    display.print(waterLevelPercentage);
    display.print("%");

    display.setTextSize(1);
    display.setCursor(0, 20);
    display.print("(");
    display.print(waterLevelCm);
    display.print(" cm)");

    display.setTextSize(1);
    display.setCursor(0, 35);
    display.print("Dist: ");
    display.print(getDistanceCm());
    display.print(" cm");

    display.display();
}

void updateBlynk() {
    Blynk.virtualWrite(V0, waterLevelPercentage); // Mức nước (%)
    Blynk.virtualWrite(V2, waterLevelCm);         // Mức nước (cm)

    // Cập nhật trạng thái đèn báo Blynk dựa trên mức nước
    if (waterLevelPercentage < LOW_WATER_THRESHOLD) {
        Blynk.virtualWrite(V1, 1); // Báo cạn (đỏ)
        Blynk.virtualWrite(V3, 0); // Không báo đầy
    } else if (waterLevelPercentage > HIGH_WATER_THRESHOLD) {
        Blynk.virtualWrite(V1, 0); // Không báo cạn
        Blynk.virtualWrite(V3, 1); // Báo đầy (xanh)
    } else {
        Blynk.virtualWrite(V1, 0); // Không báo cạn
        Blynk.virtualWrite(V3, 0); // Không báo đầy
    }

    Blynk.virtualWrite(V4, autoMode);           // Trạng thái Auto/Manual
}