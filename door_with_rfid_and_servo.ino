

#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ─── Pin & Config ──────────────────────────────────────────
#define SERVO_PIN        2      // D4
#define SERVO_OPEN       90
#define SERVO_CLOSE      0
#define DOOR_OPEN_MS     3000

// ─── Objects ───────────────────────────────────────────────
MFRC522DriverPinSimple ss_pin(15);
MFRC522DriverSPI       driver{ss_pin};
MFRC522                mfrc522{driver};
Servo                  myServo;
LiquidCrystal_I2C      lcd(0x27, 16, 2);

// ─── State ─────────────────────────────────────────────────
bool          doorOpen       = false;
unsigned long doorOpenTime   = 0;

// Running text
String        runningMsg     = "  Tempelkan kartu RFID Anda...  ";
int           scrollPos      = 0;
unsigned long lastScroll     = 0;
#define       SCROLL_MS      280   // kecepatan scroll (ms per langkah)

// ─── Whitelist UID (kosong = semua diterima) ───────────────
const byte authorizedUIDs[][4] = {
  // {0xDE, 0xAD, 0xBE, 0xEF},
};
const int NUM_AUTHORIZED = sizeof(authorizedUIDs) / sizeof(authorizedUIDs[0]);

// ─── Helpers ───────────────────────────────────────────────
bool isAuthorized(MFRC522::Uid *uid) {
  if (NUM_AUTHORIZED == 0) return true;
  for (int i = 0; i < NUM_AUTHORIZED; i++) {
    if (uid->size != 4) continue;
    bool match = true;
    for (int j = 0; j < 4; j++) {
      if (uid->uidByte[j] != authorizedUIDs[i][j]) { match = false; break; }
    }
    if (match) return true;
  }
  return false;
}

void printUID(MFRC522::Uid *uid) {
  Serial.print(F("UID: "));
  for (byte i = 0; i < uid->size; i++) {
    if (uid->uidByte[i] < 0x10) Serial.print("0");
    Serial.print(uid->uidByte[i], HEX);
    if (i < uid->size - 1) Serial.print(":");
  }
  Serial.println();
}

// ─── LCD baris 0: running text scroll ──────────────────────
void updateScrollText() {
  if (millis() - lastScroll < SCROLL_MS) return;
  lastScroll = millis();

  String visible = "";
  int len = runningMsg.length();
  for (int i = 0; i < 16; i++) {
    visible += runningMsg[(scrollPos + i) % len];
  }
  lcd.setCursor(0, 0);
  lcd.print(visible);
  scrollPos = (scrollPos + 1) % len;
}

// ─── LCD baris 1: status pintu ─────────────────────────────
void updateDoorStatus() {
  lcd.setCursor(0, 1);
  if (doorOpen) {
    lcd.print("Door: OPEN      ");
  } else {
    lcd.print("Door: CLOSED    ");
  }
}

// ─── Trigger buka pintu ────────────────────────────────────
void triggerDoor() {
  doorOpen     = true;
  doorOpenTime = millis();
  runningMsg   = "  ** AKSES DITERIMA ** Pintu terbuka...  ";
  scrollPos    = 0;
  myServo.write(SERVO_OPEN);
  Serial.println(F(">> Servo: OPEN"));
}

// ───────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Init I2C
  Wire.begin(4, 5);  // SDA=GPIO4(D2), SCL=GPIO5(D1)

  // Init LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  Initializing  ");
  lcd.setCursor(0, 1);
  lcd.print("  Please wait.. ");
  delay(1200);
  lcd.clear();

  // Init Servo
  myServo.attach(SERVO_PIN);
  myServo.write(SERVO_CLOSE);

  // Init RFID
  mfrc522.PCD_Init();
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);
  Serial.println(F("System ready."));

  updateDoorStatus();
}

// ───────────────────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  // ── 1. Tutup pintu otomatis setelah DOOR_OPEN_MS ────────
  if (doorOpen && (now - doorOpenTime >= DOOR_OPEN_MS)) {
    doorOpen   = false;
    runningMsg = "  Tempelkan kartu RFID Anda...  ";
    scrollPos  = 0;
    myServo.write(SERVO_CLOSE);
    Serial.println(F(">> Servo: CLOSE"));
    updateDoorStatus();
  }

  // ── 2. Update running text ───────────────────────────────
  updateScrollText();

  // ── 3. Cek RFID ─────────────────────────────────────────
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial())   return;

  printUID(&mfrc522.uid);

  if (isAuthorized(&mfrc522.uid)) {
    Serial.println(F("Akses diterima"));
    triggerDoor();
    updateDoorStatus();
  } else {
    Serial.println(F("Akses ditolak"));
    // Tampilkan "DITOLAK" sebentar di baris 0, lalu kembali
    lcd.setCursor(0, 0);
    lcd.print("  AKSES DITOLAK ");
    lcd.setCursor(0, 1);
    lcd.print("Door: CLOSED    ");
    delay(1500);
    scrollPos = 0;
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  delay(300);
}