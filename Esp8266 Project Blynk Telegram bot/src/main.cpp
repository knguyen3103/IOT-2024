// (Lê Thị Thảo)
// #define BLYNK_TEMPLATE_ID "TMPL65O6sDW_i"
// #define BLYNK_TEMPLATE_NAME "ESP8266"
// #define BLYNK_AUTH_TOKEN "LFlWmSFmwNYBB0y5H6WYq5lf7EMZtGxl"

// #define BOT_TOKEN "7885026566:AAGa0Tjz0dJ4C6CSyo1_iMC8KleoFYlUczI"
// #define GROUP_ID "-4684647200"
// #define ADMIN_ID "5358198549"

//(Phạm Mạnh Hoàng)
// #define BLYNK_TEMPLATE_ID "TMPL6lnJuhhnf"
// #define BLYNK_TEMPLATE_NAME "ESP8266"
// #define BLYNK_AUTH_TOKEN "kP2tbphYsFPG_14H7FCDuSdJ6juTUnXY"

// #define BOT_TOKEN "8052849841:AAH736dza0CdpWsMipOP-TWlwkA4qSWFJuo"
// #define GROUP_ID "-4615063743"
// #define ADMIN_ID "5551063619"

// (Vũ Văn Hoàng)
// #define BLYNK_TEMPLATE_ID "TMPL6EnQwI3ZN"
// #define BLYNK_TEMPLATE_NAME "Esp8266"
// #define BLYNK_AUTH_TOKEN "IJHJllpj-GsJZBpbgOsxoWjpGkIQhdS6"

// #define BOTtoken "7552149595:AAG_nU1B5MBSMpj8_fk-yUi_bXF-RCY9TIg"
// #define GROUP_ID "-4770066788"
// #define ADMIN_ID "5755242161"


//(Nguyễn Trọng Khôi Nguyên)
#define BLYNK_TEMPLATE_ID "TMPL61Qq4UXhq"
#define BLYNK_TEMPLATE_NAME "Esp8266 Project Blynk Telegram bot"
#define BLYNK_AUTH_TOKEN "YhmHYSiC3dN4WPiUz5FRc9Yq4aBm_c7j"


#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// 🔹 Cấu hình Telegram
#define BOT_TOKEN "7679532681:AAE1oPGDkkSNCUVlMcUEPsChgMrYWJdqhsw"
#define GROUP_ID "-4688474179"
#define ADMIN_ID "7173099827"

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// 🔹 Chân LED giao thông
#define gPIN 15
#define yPIN 2
#define rPIN 5

// 🔹 Chân I2C OLED
#define OLED_SDA 13
#define OLED_SCL 12

// 📟 Khởi tạo OLED SH1106
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// ⏳ Biến thời gian hoạt động
unsigned long runTime = 0;

// 🌡 Biến nhiệt độ & độ ẩm
float fTemperature = 0.0;
float fHumidity = 0.0;

// 🔁 Trạng thái đèn vàng nhấp nháy & đèn giao thông
bool yellowBlinkMode = false;
bool trafficLightOn = true;

// 🔌 Kết nối Blynk
BlynkTimer timer;

// 📶 Bắt sự kiện Switch từ Blynk (V3)
BLYNK_WRITE(V3) {
  yellowBlinkMode = param.asInt();
}

// 📟 Hiển thị trên OLED
void updateOLED() {
  oled.clearBuffer();
  oled.setFont(u8g2_font_unifont_t_vietnamese2);

  oled.drawUTF8(0, 14, ("Nhiet do: " + String(fTemperature, 1) + "°C").c_str());
  oled.drawUTF8(0, 28, ("Do am: " + String(fHumidity, 1) + "%").c_str());
  oled.drawUTF8(0, 42, ("Thoi gian: " + String(runTime) + "s").c_str());

  oled.sendBuffer();
}

// 🚥 Điều khiển đèn giao thông
void TrafficLightControl() {
  if (!trafficLightOn) {
    digitalWrite(rPIN, LOW);
    digitalWrite(yPIN, LOW);
    digitalWrite(gPIN, LOW);
    return;
  }

  static unsigned long lastTimer = 0;
  static int state = 0;
  static const unsigned long durations[] = {5000, 4000, 2000}; // Đỏ 10s, Xanh 8s, Vàng 3s
  static const int ledPins[] = {rPIN, gPIN, yPIN};

  if (yellowBlinkMode) {
    if (millis() - lastTimer > 500) {
      lastTimer = millis();
      digitalWrite(yPIN, !digitalRead(yPIN));
    }
    digitalWrite(rPIN, LOW);
    digitalWrite(gPIN, LOW);
    return;
  }

  if (millis() - lastTimer > durations[state]) {
    lastTimer = millis();
    digitalWrite(ledPins[state], LOW);
    state = (state + 1) % 3;
    digitalWrite(ledPins[state], HIGH);
  }
}

// 📟 Gửi dữ liệu lên Blynk
void sendToBlynk() {
  if (WiFi.status() == WL_CONNECTED) {
    Blynk.virtualWrite(V0, runTime);
    Blynk.virtualWrite(V1, fTemperature);
    Blynk.virtualWrite(V2, fHumidity);
  }
}

// 🔢 Sinh dữ liệu nhiệt độ & độ ẩm ngẫu nhiên
float randomTemperature() {
  return random(-400, 800) / 10.0;
}

float randomHumidity() {
  return random(0, 1000) / 10.0;
}

// 🌡 Cập nhật nhiệt độ & độ ẩm
void updateSensorData() {
  fTemperature = randomTemperature();
  fHumidity = randomHumidity();
}

// 🔔 Gửi cảnh báo Telegram mỗi 5 phút nếu vượt ngưỡng sức khỏe
void sendAlertTelegram() {
  static unsigned long lastAlert = 0;
  if (millis() - lastAlert < 300000) return; // 5 phút gửi 1 lần
  lastAlert = millis();

  String message = "";

  if (fTemperature < 10)
    message += "⚠️ Cảnh báo: Nhiệt độ quá thấp!\n";
  else if (fTemperature > 35)
    message += "🚨 Cảnh báo: Nhiệt độ quá cao!\n";

  if (fHumidity < 30)
    message += "⚠️ Cảnh báo: Độ ẩm quá thấp!\n";
  else if (fHumidity > 80)
    message += "🚨 Cảnh báo: Độ ẩm quá cao!\n";

  if (!message.isEmpty()) {
    message += "\n🌡 *Nhiệt độ:* " + String(fTemperature, 1) + "°C";
    message += "\n💧 *Độ ẩm:* " + String(fHumidity, 1) + "%";
    bot.sendMessage(GROUP_ID, message, "Markdown");
  }
}

// 🔢 Hiển thị thời gian chạy
void updateRunTime() {
  runTime++;
}

// 📩 Xử lý tin nhắn Telegram
void handleNewMessages() {
  int messageCount = bot.getUpdates(bot.last_message_received + 1);
  for (int i = 0; i < messageCount; i++) {
    String text = bot.messages[i].text;
    String sender_id = String(bot.messages[i].from_id);

    if (sender_id != ADMIN_ID) {
      bot.sendMessage(GROUP_ID, "⛔ Bạn không có quyền điều khiển bot!", "");
      return;
    }

    if (text == "/traffic_off") {
      trafficLightOn = false;
      bot.sendMessage(GROUP_ID, "🚦 Đèn giao thông đã tắt.", "");
    } else if (text == "/traffic_on") {
      trafficLightOn = true;
      bot.sendMessage(GROUP_ID, "🚦 Đèn giao thông hoạt động bình thường.", "");
    }
  }
}

// 🏁 SETUP
void setup() {
  Serial.begin(115200);
  pinMode(gPIN, OUTPUT);
  pinMode(yPIN, OUTPUT);
  pinMode(rPIN, OUTPUT);
  digitalWrite(rPIN, HIGH);

  Wire.begin(OLED_SDA, OLED_SCL);
  oled.begin();

  WiFi.begin("CNTT-MMT", "13572468");
  while (WiFi.status() != WL_CONNECTED) delay(500);

  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect();
  client.setInsecure();

  timer.setInterval(1000L, sendToBlynk); // Cập nhật dữ liệu mỗi giây
}

void loop() {
  Blynk.run();
  timer.run();
  handleNewMessages();
  TrafficLightControl();
  updateSensorData();
  sendAlertTelegram();
  updateRunTime();
  updateOLED();
}
