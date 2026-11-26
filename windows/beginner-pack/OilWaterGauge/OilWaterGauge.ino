#include <Arduino.h>
#include <U8g2lib.h>

// OLED: ZJY270S0700XG31 — SW SPI + ZJY remap
// CS=D9, RST=D6, SCL=D5, DC=D4, SDA=D3
// Analog: A0=oil temp, A1=oil pressure, A2=water temp
constexpr uint8_t PIN_OLED_CS = 9;
constexpr uint8_t PIN_OLED_RST = 6;
constexpr uint8_t PIN_OLED_SCL = 5;
constexpr uint8_t PIN_OLED_DC = 4;
constexpr uint8_t PIN_OLED_SDA = 3;

constexpr uint8_t PIN_OIL_TEMP = A0;
constexpr uint8_t PIN_OIL_PRESS = A1;
constexpr uint8_t PIN_WATER_TEMP = A2;

constexpr int ADC_MAX = 1023;
// Internal 1.1 V bandgap vs AVCC. Tune if Serial Vcc disagrees with a DMM on 5V.
constexpr float BANDGAP_VOLTS = 1.1f;
constexpr float ADC_VREF_FALLBACK = 5.0f;

// Sensor linear output is 0.6..4.1V -> 0..1000 kPa, then convert to kg/cm^2.
constexpr float OIL_P_V0 = 0.600f;
constexpr float OIL_P_V1 = 4.100f;
constexpr float OIL_P_KPA0 = 0.0f;
constexpr float OIL_P_KPA1 = 1000.0f;
constexpr float KPA_PER_KGCM2 = 98.0665f;

constexpr uint8_t TEMP_POINTS = 16;
const float TEMP_V[TEMP_POINTS] PROGMEM = {
    1.30f, 1.47f, 1.65f, 1.83f, 2.00f, 2.17f, 2.34f, 2.51f,
    2.69f, 2.88f, 3.06f, 3.24f, 3.42f, 3.61f, 3.80f, 4.00f};
const float TEMP_C[TEMP_POINTS] PROGMEM = {
    -20.0f, -10.0f, 0.0f, 10.0f, 20.0f, 30.0f, 40.0f, 50.0f,
    60.0f, 70.0f, 80.0f, 90.0f, 100.0f, 110.0f, 120.0f, 130.0f};

U8G2_SSD1322_ZJY_256X64_1_4W_SW_SPI u8g2(
    U8G2_R0,
    PIN_OLED_SCL,
    PIN_OLED_SDA,
    PIN_OLED_CS,
    PIN_OLED_DC,
    PIN_OLED_RST);

struct SensorReading {
  float oil_temp_c;
  float water_temp_c;
  float oil_press_kgcm2;
  float oil_temp_v;
  float water_temp_v;
  float oil_press_v;
  float avcc_v;
  int oil_temp_raw;
  int water_temp_raw;
  int oil_press_raw;
};

// Discard first sample after mux switch, then average.
static int readAdcStable(uint8_t pin) {
  analogRead(pin);
  delayMicroseconds(50);
  long sum = 0;
  for (uint8_t i = 0; i < 4; ++i) {
    sum += analogRead(pin);
  }
  return static_cast<int>(sum / 4);
}

// Measure AVCC from the 1.1 V bandgap. DEFAULT analogReference tracks AVCC,
// so a hardcoded 5.0 V makes temps read high when USB/regulator sag.
static float readAvccVolts() {
  const uint8_t old_mux = ADMUX;
  ADMUX = (1 << REFS0) | (1 << MUX3) | (1 << MUX2) | (1 << MUX1);
  delayMicroseconds(250);
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC)) {
  }
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC)) {
  }
  const uint16_t raw = ADC;
  ADMUX = old_mux;
  if (raw < 16) {
    return ADC_VREF_FALLBACK;
  }
  return (BANDGAP_VOLTS * static_cast<float>(ADC_MAX)) / raw;
}

static float readVoltage(uint8_t pin, float vref, int* raw_out = nullptr) {
  const int raw = readAdcStable(pin);
  if (raw_out != nullptr) {
    *raw_out = raw;
  }
  return (raw * vref) / ADC_MAX;
}

static float oilPressureKpaFromVolt(float v) {
  if (v <= OIL_P_V0) {
    return OIL_P_KPA0;
  }
  if (v >= OIL_P_V1) {
    return OIL_P_KPA1;
  }
  return OIL_P_KPA0 +
         (v - OIL_P_V0) * (OIL_P_KPA1 - OIL_P_KPA0) / (OIL_P_V1 - OIL_P_V0);
}

static float tempFromVolt(float v) {
  const float v0 = pgm_read_float(&TEMP_V[0]);
  const float c0 = pgm_read_float(&TEMP_C[0]);
  const float vN = pgm_read_float(&TEMP_V[TEMP_POINTS - 1]);
  const float cN = pgm_read_float(&TEMP_C[TEMP_POINTS - 1]);

  if (v <= v0) {
    return c0;
  }
  if (v >= vN) {
    return cN;
  }

  for (uint8_t i = 0; i < TEMP_POINTS - 1; ++i) {
    const float va = pgm_read_float(&TEMP_V[i]);
    const float vb = pgm_read_float(&TEMP_V[i + 1]);
    if (v >= va && v <= vb) {
      const float ca = pgm_read_float(&TEMP_C[i]);
      const float cb = pgm_read_float(&TEMP_C[i + 1]);
      return ca + (v - va) * (cb - ca) / (vb - va);
    }
  }
  return cN;
}

static void drawBootScreen() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_helvB14_tr);
    u8g2.drawStr(8, 40, "Oil / Water");
  } while (u8g2.nextPage());
}

static void drawSensorCell(uint8_t x, const __FlashStringHelper* label, float value,
                           uint8_t decimals, const __FlashStringHelper* unit) {
  u8g2.setFont(u8g2_font_helvB12_tr);
  u8g2.setCursor(x, 14);
  u8g2.print(label);

  u8g2.setFont(u8g2_font_logisoso24_tr);
  u8g2.setCursor(x, 52);
  u8g2.print(value, decimals);
  u8g2.setFont(u8g2_font_helvB10_tr);
  u8g2.print(unit);
}

static void drawSensors(const SensorReading& s) {
  u8g2.firstPage();
  do {
    drawSensorCell(2, F("Water"), s.water_temp_c, 0, F("C"));
    drawSensorCell(90, F("OilT"), s.oil_temp_c, 0, F("C"));
    // Compact unit on screen; value is kg/cm^2
    drawSensorCell(176, F("OilP"), s.oil_press_kgcm2, 1, F("kg"));
  } while (u8g2.nextPage());
}

// Timer1 CTC @ 16MHz / 256 -> 0.5 s base tick (OCR1A = 31249)
// Oil pressure: every tick (0.5 s). Temps: every 4 ticks (2.0 s).
volatile bool g_update_due = false;

ISR(TIMER1_COMPA_vect) {
  g_update_due = true;
}

static void startUpdateTimer() {
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 31249;
  TCCR1B |= (1 << WGM12);  // CTC
  TCCR1B |= (1 << CS12);   // clk/256
  TIMSK1 |= (1 << OCIE1A); // compare A interrupt
  interrupts();
}

void setup() {
  Serial.begin(115200);
  analogReference(DEFAULT);

  u8g2.begin();
  u8g2.setContrast(0x7F);
  drawBootScreen();

  startUpdateTimer();
  g_update_due = true;  // first draw immediately

  Serial.println(F("sensors: A0=oilT A1=oilP A2=waterT Vcc=bandgap oilP=0.5s temps=2s"));
}

void loop() {
  static SensorReading s = {};
  static uint8_t tick = 0;

  if (!g_update_due) {
    return;
  }
  g_update_due = false;

  // Always refresh oil pressure (0.5 s).
  s.avcc_v = readAvccVolts();
  analogRead(PIN_OIL_PRESS);  // settle mux after bandgap
  s.oil_press_v = readVoltage(PIN_OIL_PRESS, s.avcc_v, &s.oil_press_raw);
  s.oil_press_kgcm2 = oilPressureKpaFromVolt(s.oil_press_v) / KPA_PER_KGCM2;

  // Temps every 2.0 s (every 4th tick), including the first frame.
  if ((tick % 4) == 0) {
    s.oil_temp_v = readVoltage(PIN_OIL_TEMP, s.avcc_v, &s.oil_temp_raw);
    s.water_temp_v = readVoltage(PIN_WATER_TEMP, s.avcc_v, &s.water_temp_raw);
    s.oil_temp_c = tempFromVolt(s.oil_temp_v);
    s.water_temp_c = tempFromVolt(s.water_temp_v);
  }
  ++tick;

  drawSensors(s);

  Serial.print(F("W="));
  Serial.print(s.water_temp_c, 0);
  Serial.print(F("C("));
  Serial.print(s.water_temp_v, 2);
  Serial.print(F("V) OilT="));
  Serial.print(s.oil_temp_c, 0);
  Serial.print(F("C("));
  Serial.print(s.oil_temp_v, 2);
  Serial.print(F("V) OilP="));
  Serial.print(s.oil_press_kgcm2, 1);
  Serial.print(F("kg("));
  Serial.print(s.oil_press_v, 2);
  Serial.print(F("V) Vcc="));
  Serial.print(s.avcc_v, 3);
  Serial.print(F("V raw="));
  Serial.print(s.water_temp_raw);
  Serial.print(',');
  Serial.print(s.oil_temp_raw);
  Serial.print(',');
  Serial.println(s.oil_press_raw);
}
