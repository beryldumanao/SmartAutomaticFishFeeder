#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>
#include <EEPROM.h>

Servo servo;
ESP8266WebServer server(80);

// ===== HOTSPOT =====
const char* ssid = "Project Beryl & Davinza";
const char* password = "12345678";

// ===== JADWAL =====
#define MAX_SCHEDULE 5
int feedHour[MAX_SCHEDULE];
int feedMinute[MAX_SCHEDULE];
bool scheduleActive[MAX_SCHEDULE];

// ===== WAKTU =====
int currentHour = 0;
int currentMinute = 0;
unsigned long lastMinuteTick = 0;

// ===== SERVO =====
unsigned long servoDuration = 500;  // default 500 ms

// ===== EEPROM =====
#define EEPROM_START 0

// ================= SERVO =================
void feedNow() {
  Serial.println("SERVO GERAK");
  servo.write(90);
  delay(servoDuration);
  servo.write(0);
}

// ================= EEPROM =================
void saveAllToEEPROM() {
  for (int i = 0; i < MAX_SCHEDULE; i++) {
    EEPROM.write(EEPROM_START + (i * 3), feedHour[i]);
    EEPROM.write(EEPROM_START + (i * 3) + 1, feedMinute[i]);
    EEPROM.write(EEPROM_START + (i * 3) + 2, scheduleActive[i]);
  }
  EEPROM.commit();
}

void loadFromEEPROM() {
  for (int i = 0; i < MAX_SCHEDULE; i++) {
    feedHour[i] = EEPROM.read(EEPROM_START + (i * 3));
    feedMinute[i] = EEPROM.read(EEPROM_START + (i * 3) + 1);
    scheduleActive[i] = EEPROM.read(EEPROM_START + (i * 3) + 2);

    if (feedHour[i] > 23 || feedMinute[i] > 59) {
      scheduleActive[i] = false;
    }
  }
}

// ================= HANDLER =================
void handleFeed() {
  server.send(200, "text/plain", "OK");  // kirim respon dulu
  delay(50);                             // beri waktu kirim data
  feedNow();                             // baru gerakkan servo
}

void handleSetTime() {
  if (server.hasArg("slot") && server.hasArg("hour") && server.hasArg("minute")) {

    int slot = server.arg("slot").toInt();

    if (slot >= 0 && slot < MAX_SCHEDULE) {

      feedHour[slot] = server.arg("hour").toInt();
      feedMinute[slot] = server.arg("minute").toInt();
      scheduleActive[slot] = true;

      saveAllToEEPROM();

      Serial.print("Slot ");
      Serial.print(slot + 1);
      Serial.print(" diset: ");
      Serial.print(feedHour[slot]);
      Serial.print(":");
      Serial.println(feedMinute[slot]);

      server.send(200, "text/plain", "JADWAL TERSIMPAN");
    }
  }
}

void handleClearTime() {
  if (server.hasArg("slot")) {

    int slot = server.arg("slot").toInt();

    if (slot >= 0 && slot < MAX_SCHEDULE) {

      scheduleActive[slot] = false;
      feedHour[slot] = 255;
      feedMinute[slot] = 255;

      saveAllToEEPROM();

      Serial.println("JADWAL DIHAPUS");

      server.send(200, "text/plain", "JADWAL DIHAPUS");
    }
  }
}

void handleSyncTime() {
  if (server.hasArg("hour") && server.hasArg("minute")) {

    currentHour = server.arg("hour").toInt();
    currentMinute = server.arg("minute").toInt();

    Serial.print("Waktu sinkron: ");
    Serial.print(currentHour);
    Serial.print(":");
    Serial.println(currentMinute);

    server.send(200, "text/plain", "WAKTU DISINKRON");
  }
}

void handleSetSpeed() {
  if (server.hasArg("durasi")) {

    servoDuration = server.arg("durasi").toInt();

    Serial.print("Durasi servo: ");
    Serial.println(servoDuration);

    server.send(200, "text/plain", "KECEPATAN DISET");
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  EEPROM.begin(512);

  servo.attach(D5);
  servo.write(0);

  WiFi.softAP(ssid, password);

  Serial.println("Hotspot aktif");
  Serial.println(WiFi.softAPIP());

  loadFromEEPROM();

  server.on("/feed", handleFeed);
  server.on("/setTime", handleSetTime);
  server.on("/clearTime", handleClearTime);
  server.on("/syncTime", handleSyncTime);
  server.on("/setSpeed", handleSetSpeed);

  server.begin();
}

// ================= LOOP =================
void loop() {
  server.handleClient();

  // Hitung waktu setiap 1 menit
  if (millis() - lastMinuteTick >= 60000) {
    lastMinuteTick = millis();

    currentMinute++;

    if (currentMinute >= 60) {
      currentMinute = 0;
      currentHour++;
      if (currentHour >= 24) currentHour = 0;
    }

    Serial.print("Waktu: ");
    Serial.print(currentHour);
    Serial.print(":");
    Serial.println(currentMinute);
  }

  // Cek jadwal
  for (int i = 0; i < MAX_SCHEDULE; i++) {

    if (scheduleActive[i] &&
        currentHour == feedHour[i] &&
        currentMinute == feedMinute[i]) {

      Serial.print("WAKTU MAKAN SLOT ");
      Serial.println(i + 1);

      feedNow();

      scheduleActive[i] = false;  // supaya tidak double
      saveAllToEEPROM();
    }
  }
}
