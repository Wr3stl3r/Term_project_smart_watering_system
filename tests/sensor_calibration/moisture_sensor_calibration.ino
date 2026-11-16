/*
  Smart Watering System - Arduino Nano
  Capacitive soil moisture sensor on A0, pump switched by a relay/MOSFET on D7.

  Behavior:
   - Reads soil moisture every second (averaged to reduce noise)
   - Starts the pump when soil is DRY, stops when soil is WET (hysteresis)
   - Pump runs in short bursts, then waits for water to soak in (cooldown)
   - Hard cap on pump run time as a safety cutoff
*/

// ---------- Pins ----------
const int SENSOR_PIN = A0;
const int PUMP_PIN   = 7;

// Set to HIGH for a MOSFET/transistor driver or an active-high relay board.
// Set to LOW for an active-low relay board (most common 5V relay modules).
const int PUMP_ON  = HIGH;
const int PUMP_OFF = (PUMP_ON == HIGH) ? LOW : HIGH;

// ---------- Calibration (EDIT THESE after measuring sensor) ----------
// Capacitive sensors read HIGHER when dry, LOWER when wet.
// Measure: sensor in open air (or dry soil) and sensor in a glass of water.
const int DRY_VALUE = 550;   // reading in dry soil
const int WET_VALUE = 203;   // reading in wet soil

// Moisture percentage thresholds (hysteresis band)
const int START_BELOW_PCT = 30;  // start watering below this
const int STOP_ABOVE_PCT  = 45;  // stop watering above this

// ---------- Timing ----------
const unsigned long READ_INTERVAL_MS   = 1000;
const unsigned long MAX_PUMP_RUN_MS    = 10000UL;   // 10 s max per burst
const unsigned long COOLDOWN_MS        = 600000UL;  // 10 min between bursts

// ---------- State ----------
bool pumpOn = false;
unsigned long pumpStartedAt = 0;
unsigned long pumpStoppedAt = 0;
unsigned long lastRead = 0;
bool everWatered = false;

int readMoisturePct() {
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(SENSOR_PIN);
    delay(5);
  }
  int raw = sum / 10;
  int pct = map(raw, DRY_VALUE, WET_VALUE, 0, 100);
  return constrain(pct, 0, 100);
}

void setPump(bool on) {
  pumpOn = on;
  digitalWrite(PUMP_PIN, on ? PUMP_ON : PUMP_OFF);
  if (on) pumpStartedAt = millis();
  else    { pumpStoppedAt = millis(); everWatered = true; }
}

void setup() {
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, PUMP_OFF);   // make sure pump starts OFF
  Serial.begin(9600);
  Serial.println(F("Smart Watering System starting"));
}

void loop() {
  unsigned long now = millis();
  if (now - lastRead < READ_INTERVAL_MS) return;
  lastRead = now;

  int pct = readMoisturePct();
  Serial.print(F("Moisture: "));
  Serial.print(pct);
  Serial.print(F("%  Pump: "));
  Serial.println(pumpOn ? F("ON") : F("OFF"));
  Serial.println(analogRead(SENSOR_PIN)); // this will return the values the sensor is reading, use these to calibrate DRY_VALUE and WET_VALUE

  if (pumpOn) {
    // Stop if soil is wet enough OR safety timer expires
    if (pct >= STOP_ABOVE_PCT || (now - pumpStartedAt) >= MAX_PUMP_RUN_MS) {
      setPump(false);
    }
  } else {
    bool cooledDown = !everWatered || (now - pumpStoppedAt) >= COOLDOWN_MS;
    if (pct <= START_BELOW_PCT && cooledDown) {
      setPump(true);
    }
  }
}
