#define BLYNK_TEMPLATE_ID "TMPL6Yop0LsMG"
#define BLYNK_TEMPLATE_NAME "Blynk Traffic and DHT Sensor"
#define BLYNK_AUTH_TOKEN "qocvZhpP_PTjS6FkwktTLIZQNCbAO3br"

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

#define gPIN 15
#define yPIN 2
#define rPIN 5

#define OLED_SDA 13
#define OLED_SCL 12

// WiFi SSID và mật khẩu
char ssid[] = "Wokwi-GUEST";
char pass[] = "";  // Không có mật khẩu

bool yellowBlinkMode = false;
unsigned long startTime;

U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
BlynkTimer timer;

// Khai báo trước các hàm để tránh lỗi 'not declared in this scope'
void updateSensorData();
void updateTrafficLight();

// Nhận dữ liệu từ Switch trên Blynk (V1)
BLYNK_WRITE(V1) {
  yellowBlinkMode = param.asInt();
}

void setup() {
  Serial.begin(115200);
  
  pinMode(gPIN, OUTPUT);
  pinMode(yPIN, OUTPUT);
  pinMode(rPIN, OUTPUT);
  
  digitalWrite(gPIN, LOW);
  digitalWrite(yPIN, LOW);
  digitalWrite(rPIN, LOW);

  Wire.begin(OLED_SDA, OLED_SCL);
  oled.begin();
  
  oled.clearBuffer();
  oled.setFont(u8g2_font_unifont_t_vietnamese1);
  oled.drawUTF8(0, 14, "Trường ĐHKH");  
  oled.drawUTF8(0, 28, "Khoa CNTT");
  oled.drawUTF8(0, 42, "Lập trình IoT");  
  oled.sendBuffer();
  
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  startTime = millis();
  timer.setInterval(2000L, updateSensorData);
  timer.setInterval(1000L, updateTrafficLight);
}

void updateSensorData() {
  float temperature = random(-400, 800) / 10.0;  // Sinh nhiệt độ từ -40.0 đến 80.0
  float humidity = random(0, 1000) / 10.0;  // Sinh độ ẩm từ 0.0% đến 100.0%

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" *C");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  oled.clearBuffer();
  oled.setFont(u8g2_font_unifont_t_vietnamese2);
  
  String s = String("Nhiet do: ") + String(temperature, 2) + " °C";
  oled.drawUTF8(0, 14, s.c_str());

  s = String("Do am: ") + String(humidity, 2) + " %";
  oled.drawUTF8(0, 42, s.c_str());

  oled.sendBuffer();

  Blynk.virtualWrite(V2, temperature);
  Blynk.virtualWrite(V3, humidity);

  long elapsedTime = (millis() - startTime) / 1000;
  Blynk.virtualWrite(V0, elapsedTime);
}

void updateTrafficLight() {
  static int currentLed = 0;  
  static unsigned long lastChange = 0;

  if (yellowBlinkMode) {
    digitalWrite(gPIN, LOW);
    digitalWrite(rPIN, LOW);
    digitalWrite(yPIN, millis() % 1000 < 500 ? HIGH : LOW);
    return;
  }

  unsigned long now = millis();
  if (now - lastChange >= (currentLed == 0 ? 8000 : (currentLed == 1 ? 3000 : 10000))) {
    lastChange = now;
    currentLed = (currentLed + 1) % 3;
    
    digitalWrite(gPIN, currentLed == 0);
    digitalWrite(yPIN, currentLed == 1);
    digitalWrite(rPIN, currentLed == 2);
  }
}

void loop() {
  Blynk.run();
  timer.run();
}
