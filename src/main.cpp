#include <Arduino.h>
#include <U8g2lib.h>

// OLED: ZJY270S0700XG31 — SW SPI + ZJY remap
// CS=D9, RST=D6, SCL=D5, DC=D4, SDA=D3
// Analog: A0=oil temp, A1=oil pressure, A2=water temp
//
// Display select: 0 = ZJY270S0700XG31 (SSD1322, 256x64, page buffer)
//                 1 = 2.42" SSD1309 128x64 SPI module (field test panel)
#ifndef DISPLAY_SSD1309_128X64
#define DISPLAY_SSD1309_128X64 0
#endif
// 1 = field diagnosis screens (frame/ticks/font paths). Production stays 0.
#ifndef DIAG_FIRMWARE
#define DIAG_FIRMWARE 0
#endif

constexpr uint8_t PIN_OLED_CS = 9;
constexpr uint8_t PIN_OLED_RST = 6;
constexpr uint8_t PIN_OLED_SCL = 5;
constexpr uint8_t PIN_OLED_DC = 4;
constexpr uint8_t PIN_OLED_SDA = 3;

constexpr uint8_t PIN_OIL_TEMP = A0;
constexpr uint8_t PIN_OIL_PRESS = A1;
constexpr uint8_t PIN_WATER_TEMP = A2;

constexpr int ADC_MAX = 1023;
// Internal 1.1 V bandgap vs AVCC. Calibrated against the measured 5.25 V rail.
constexpr float BANDGAP_VOLTS = 1.088f;
constexpr float ADC_VREF_FALLBACK = 5.25f;

// Sensor outputs are calibrated against the current 5.25 V reference.
// For ratiometric 5 V sensors this keeps readings stable when AVCC moves.
constexpr float SENSOR_REFERENCE_VCC = 5.25f;
constexpr bool SENSOR_OUTPUT_RATIOMETRIC = true;

// Sensor linear output is 0.6..4.1V -> 0..1000 kPa, then convert to kg/cm^2.
// Zero point measured with the connected sensor at atmospheric pressure.
constexpr float OIL_P_V0 = 0.620f;
constexpr float OIL_P_V1 = 4.100f;
constexpr float OIL_P_KPA0 = 0.0f;
constexpr float OIL_P_KPA1 = 1000.0f;
constexpr float KPA_PER_KGCM2 = 98.0665f;

// Two-point temperature calibration from the live sensor checks:
// room reference 27.7 C at 2.31 V, boiling-water reference 100 C at 3.746 V.
// Both temperature channels use this same sensor characteristic.
constexpr float TEMP_CAL_V0 = 2.310f;
constexpr float TEMP_CAL_C0 = 27.7f;
constexpr float TEMP_CAL_V1 = 3.746f;
constexpr float TEMP_CAL_C1 = 100.0f;

#if DISPLAY_SSD1309_128X64
// Page buffer (128 B) instead of the 1 KB full buffer: on a 2 KB ATmega328P the
// full buffer left ~400 B of stack and the panel showed garbage ("snow").
// SSD1309_VARIANT=2 selects the panel with x_offset=2 (some modules shift 2 px).
#ifndef SSD1309_VARIANT
#define SSD1309_VARIANT 0
#endif
#if SSD1309_VARIANT == 2
U8G2_SSD1309_128X64_NONAME2_1_4W_SW_SPI u8g2(
#else
U8G2_SSD1309_128X64_NONAME0_1_4W_SW_SPI u8g2(
#endif
    U8G2_R0,
    PIN_OLED_SCL,
    PIN_OLED_SDA,
    PIN_OLED_CS,
    PIN_OLED_DC,
    PIN_OLED_RST);
#else
U8G2_SSD1322_ZJY_256X64_1_4W_SW_SPI u8g2(
    U8G2_R0,
    PIN_OLED_SCL,
    PIN_OLED_SDA,
    PIN_OLED_CS,
    PIN_OLED_DC,
    PIN_OLED_RST);
#endif

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
  const float measured_voltage = (raw * vref) / ADC_MAX;
  if (SENSOR_OUTPUT_RATIOMETRIC && vref > 0.1f) {
    return measured_voltage * SENSOR_REFERENCE_VCC / vref;
  }
  return measured_voltage;
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
  float temperature = TEMP_CAL_C0 +
                      (v - TEMP_CAL_V0) * (TEMP_CAL_C1 - TEMP_CAL_C0) /
                          (TEMP_CAL_V1 - TEMP_CAL_V0);
  if (temperature < -20.0f) {
    return -20.0f;
  }
  if (temperature > 130.0f) {
    return 130.0f;
  }
  return temperature;
}

static void drawBootScreen() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_helvB14_tr);
    u8g2.drawStr(8, 40, "Oil / Water");
#if DIAG_FIRMWARE
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(8, 56, "DIAG 3 screens");
#endif
#if DISPLAY_SSD1309_128X64
    // Identify the field-test build on the panel itself.
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(8, 56, "SSD1309 128x64");
#endif
  } while (u8g2.nextPage());
}

#if DISPLAY_SSD1309_128X64
// 128x64 layout: three 42 px columns (0 / 43 / 86).
// label 5x7 @y=9, value logisoso18 @y=42, unit 5x7 @y=56.
// Verified with tools/oled-sim/sim128.c (real U8g2 fonts, no clipping).
static void drawSensorCell(uint8_t x, const __FlashStringHelper* label, float value,
                           uint8_t decimals, const __FlashStringHelper* unit) {
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.setCursor(x + 1, 9);
  u8g2.print(label);

  u8g2.setFont(u8g2_font_logisoso18_tr);
  u8g2.setCursor(x + 1, 42);
  u8g2.print(value, decimals);

  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.setCursor(x + 1, 56);
  u8g2.print(unit);
}
#else
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
#endif

static void drawSensors(const SensorReading& s) {
#if DISPLAY_SSD1309_128X64
  u8g2.firstPage();
  do {
    drawSensorCell(0, F("WATER"), s.water_temp_c, 0, F("C"));
    drawSensorCell(43, F("OILT"), s.oil_temp_c, 0, F("C"));
    // Compact unit on screen; value is kg/cm^2
    drawSensorCell(86, F("OILP"), s.oil_press_kgcm2, 1, F("kg"));
    u8g2.drawVLine(42, 0, 64);
    u8g2.drawVLine(85, 0, 64);
  } while (u8g2.nextPage());
#else
  u8g2.firstPage();
  do {
    drawSensorCell(2, F("Water"), s.water_temp_c, 0, F("C"));
    drawSensorCell(90, F("OilT"), s.oil_temp_c, 0, F("C"));
    // Compact unit on screen; value is kg/cm^2
    drawSensorCell(176, F("OilP"), s.oil_press_kgcm2, 1, F("kg"));
  } while (u8g2.nextPage());
#endif
}

#if DIAG_FIRMWARE
// --- one-shot field diagnosis (SSD1322 / 256x64) ----------------------------
// Screen 1 MAP : frame + 8 px ticks + bottom bar (geometry) and tall-font digits
// Screen 2 PATH: the same number via print(float), print("literal"), drawStr()
// Screen 3 LIVE: live values in helvB12 (small) and the same value in logisoso24
static uint16_t freeRamBytes() {
  extern int __heap_start;
  extern char *__brkval;
  int probe;
  int top = (__brkval == 0) ? (int)&__heap_start : (int)__brkval;
  return (uint16_t)((int)&probe - top);
}

static void drawDiag(uint8_t screen, const SensorReading& s) {
  u8g2.firstPage();
  do {
    if (screen == 0) {
      u8g2.drawFrame(0, 0, 256, 64);
      for (uint8_t y = 0; y < 64; y += 8) {
        u8g2.drawHLine(0, y, 6);
      }
      u8g2.drawBox(0, 62, 256, 2);  // must sit on the last visible row

      u8g2.setFont(u8g2_font_5x7_tr);
      u8g2.setCursor(10, 10);
      u8g2.print(F("1/3 MAP  Vcc="));
      u8g2.print(s.avcc_v, 2);
      u8g2.print(F(" free="));
      u8g2.print(freeRamBytes());

      u8g2.setFont(u8g2_font_logisoso24_tr);
      u8g2.setCursor(10, 48);
      u8g2.print(F("0123456789"));

      u8g2.setFont(u8g2_font_5x7_tr);
      u8g2.setCursor(10, 58);
      u8g2.print(F("nan inf kg/cm2  base 48"));
    } else if (screen == 1) {
      u8g2.setFont(u8g2_font_5x7_tr);
      u8g2.setCursor(4, 10);
      u8g2.print(F("2/3 PATH  fixed value 12.3"));

      u8g2.setFont(u8g2_font_helvB12_tr);
      u8g2.setCursor(4, 26);
      u8g2.print(F("print"));
      u8g2.setCursor(90, 26);
      u8g2.print(F("str"));
      u8g2.setCursor(176, 26);
      u8g2.print(F("draw"));

      u8g2.setFont(u8g2_font_logisoso24_tr);
      u8g2.setCursor(4, 58);
      u8g2.print(12.3f, 1);  // Print::printFloat path (what the gauge uses)
      u8g2.setCursor(90, 58);
      u8g2.print(F("12.3"));  // flash string, same font
      u8g2.drawStr(176, 58, "12.3");  // direct draw, no Print
    } else {
      u8g2.setFont(u8g2_font_5x7_tr);
      u8g2.setCursor(4, 10);
      u8g2.print(F("3/3 LIVE  Vcc="));
      u8g2.print(s.avcc_v, 2);

      u8g2.setFont(u8g2_font_helvB12_tr);
      u8g2.setCursor(4, 26);
      u8g2.print(F("W="));
      u8g2.print(s.water_temp_c, 0);
      u8g2.print(F(" T="));
      u8g2.print(s.oil_temp_c, 0);
      u8g2.print(F(" P="));
      u8g2.print(s.oil_press_kgcm2, 1);

      u8g2.setFont(u8g2_font_5x7_tr);
      u8g2.setCursor(4, 36);
      u8g2.print(F("raw "));
      u8g2.print(s.water_temp_raw);
      u8g2.print(',');
      u8g2.print(s.oil_temp_raw);
      u8g2.print(',');
      u8g2.print(s.oil_press_raw);

      u8g2.setFont(u8g2_font_logisoso24_tr);
      u8g2.setCursor(4, 62);
      u8g2.print(s.water_temp_c, 0);  // same value, tall font

      u8g2.setFont(u8g2_font_5x7_tr);
      u8g2.setCursor(96, 62);
      u8g2.print(F("<- same W, logisoso24"));
    }
  } while (u8g2.nextPage());
}
#endif

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

#if DIAG_FIRMWARE
  drawDiag((uint8_t)((tick / 6) % 3), s);  // 3 s per screen
#else
  drawSensors(s);
#endif

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
#if DIAG_FIRMWARE
  Serial.print(s.oil_press_raw);
  Serial.print(F(" free="));
  Serial.print(freeRamBytes());
  Serial.print(F(" scr="));
  Serial.print((tick / 6) % 3);
  Serial.println();
#else
  Serial.println(s.oil_press_raw);
#endif
}
