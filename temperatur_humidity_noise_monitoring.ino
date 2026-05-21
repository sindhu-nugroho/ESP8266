/*
  ESP8266 NodeMCU — DHT11 + Sound Sensor + LCD I2C
  ─────────────────────────────────────────────────
  WIRING:
    DHT11  DATA → D0 (GPIO16) | VCC → 3.3V | GND → GND
    Sound  AO   → A0          | VCC → 3.3V | GND → GND
    LCD    SDA  → D2 (GPIO4)  | SCL → D1 (GPIO5)
           VCC  → 3.3V        | GND → GND
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// ─── Pin & Config ──────────────────────────────────────────
#define DHT_PIN         16      // D0
#define DHT_TYPE        DHT11
#define SOUND_PIN       A0

#define SOUND_THRESHOLD 400     // 0–1023, turunkan = lebih sensitif
#define NOISE_WARN_MS   3000    // lama peringatan tampil (ms)
#define SENSOR_READ_MS  2000    // interval baca DHT (ms)

// ─── Objects ───────────────────────────────────────────────
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT               dht(DHT_PIN, DHT_TYPE);

// ─── State ─────────────────────────────────────────────────
float         lastTemp     = 0;
float         lastHum      = 0;
bool          noiseWarning = false;
unsigned long noiseWarnTime  = 0;
unsigned long lastSensorRead = 0;

// ─── Tampilkan suhu & kelembaban di baris 0 ────────────────
void showTempHum(float t, float h) {
  lcd.setCursor(0, 0);
  if (isnan(t) || isnan(h)) {
    lcd.print("Sensor error!   ");
    return;
  }
  // Format: "T:28.5C  H:72%  "
  String line = "T:";
  line += String(t, 1);
  line += "C  H:";
  line += String((int)h);
  line += "%  ";
  while (line.length() < 16) line += " ";
  lcd.print(line.substring(0, 16));
}

// ─── Tampilkan status di baris 1 ───────────────────────────
void showStatus() {
  lcd.setCursor(0, 1);
  if (noiseWarning) {
    lcd.print("!! JANGAN BERISIK");  // 16 char pas
  } else {
    lcd.print("Suasana tenang  ");
  }
}

// ───────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  // Init I2C & LCD
  Wire.begin(4, 5);   // SDA=GPIO4(D2), SCL=GPIO5(D1)
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Room  Monitor  ");
  lcd.setCursor(0, 1);
  lcd.print("  Starting...   ");
  delay(1200);
  lcd.clear();

  // Init DHT
  dht.begin();

  Serial.println(F("System ready."));
}

// ───────────────────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  // ── 1. Baca sensor suara setiap loop ─────────────────────
  int soundVal = analogRead(SOUND_PIN);
  Serial.print(F("Sound: ")); Serial.println(soundVal);

  if (soundVal > SOUND_THRESHOLD) {
    noiseWarning  = true;
    noiseWarnTime = now;
    Serial.println(F("!! Noise detected"));
  }

  // ── 2. Reset peringatan setelah NOISE_WARN_MS ────────────
  if (noiseWarning && (now - noiseWarnTime >= NOISE_WARN_MS)) {
    noiseWarning = false;
  }

  // ── 3. Baca DHT setiap SENSOR_READ_MS ───────────────────
  if (now - lastSensorRead >= SENSOR_READ_MS) {
    lastSensorRead = now;

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (!isnan(t) && !isnan(h)) {
      lastTemp = t;
      lastHum  = h;
    }

    Serial.print(F("Temp: ")); Serial.print(lastTemp);
    Serial.print(F("C  Hum: ")); Serial.println(lastHum);

    showTempHum(lastTemp, lastHum);
  }

  // ── 4. Update baris 1 LCD setiap loop ───────────────────
  showStatus();

  delay(100);   // polling ~10x per detik
}
