#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>

#define TOUCH_PIN   13    // D7 (GPIO13)
#define SERVO_PIN   12    // D6 (GPIO12)
#define SCREEN_W    128
#define SCREEN_H    64
#define OLED_ADDR   0x3C  // coba 0x3D jika tidak muncul

Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);
Servo myServo;

bool doorOpen = false;
unsigned long openTime = 0;
const unsigned long OPEN_DURATION = 3000;

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(TOUCH_PIN, INPUT);

  Wire.begin(4, 5); // SDA=D2(GPIO4), SCL=D1(GPIO5)

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED tidak ditemukan! Cek koneksi.");
    while (true); // berhenti di sini jika OLED gagal
  }

  display.clearDisplay();
  display.display();

  showStandby();
  Serial.println("System ready.");
}

void loop() {
  int touchValue = digitalRead(TOUCH_PIN);

  if (touchValue == HIGH && !doorOpen) {
    openDoor();
  }

  if (doorOpen && (millis() - openTime >= OPEN_DURATION)) {
    closeDoor();
  }
}

void openDoor() {
  doorOpen = true;
  openTime = millis();

  myServo.attach(SERVO_PIN);
  myServo.write(180);
  delay(700);

  // Tampilan OLED: OPEN
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  // Ikon gembok terbuka (kotak + tanda)
  display.drawRoundRect(44, 8, 40, 30, 4, SSD1306_WHITE);
  display.fillRect(52, 14, 24, 18, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(58, 20);
  display.print("OPEN");

  // Teks besar UNLOCKED
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(8, 44);
  display.print("UNLOCKED");

  display.display();
  Serial.println("Door OPEN");
}

void closeDoor() {
  doorOpen = false;

  myServo.write(0);
  delay(700);
  myServo.detach();

  showStandby();
  Serial.println("Door CLOSED");
}

void showStandby() {
  display.clearDisplay();

  // Ikon gembok tertutup
  display.setTextColor(SSD1306_WHITE);
  display.drawRoundRect(44, 4, 40, 30, 4, SSD1306_WHITE);
  display.drawRoundRect(50, 0, 28, 16, 8, SSD1306_WHITE); // lengkung atas
  display.fillRect(52, 8, 24, 20, SSD1306_WHITE);         // badan gembok
  display.fillCircle(64, 20, 4, SSD1306_BLACK);            // lubang kunci

  // Teks LOCKED
  display.setTextSize(2);
  display.setCursor(16, 40);
  display.print("  LOCKED");

  display.setTextSize(1);
  display.setCursor(20, 57);
  display.print("Touch to unlock");

  display.display();
}
