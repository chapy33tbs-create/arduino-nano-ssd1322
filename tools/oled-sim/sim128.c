/* 128x64 layout simulator for the SSD1309 2.42" module (field test panel).
 * Renders the same cells as src/main.cpp (DISPLAY_SSD1309_128X64=1) using the
 * real U8g2 2.36.18 fonts. The SSD1309 128x64 noname0 driver only exists in
 * full-buffer mode (_f), which is what the firmware uses for this panel.
 *   mode 1: normal values  -> one 128x64 PBM
 *   mode 2: value strings forced empty (reproduces the blank-value bug)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "u8g2.h"

static u8g2_t u8g2;
static int force_empty = 0;
static void out_cb(const char *s) { fputs(s, stdout); }

static void print_float(double number, uint8_t digits, char *buf) {
  int n = 0; buf[0] = 0;
  if (force_empty) return;
  if (isnan(number)) { strcpy(buf, "nan"); return; }
  if (isinf(number)) { strcpy(buf, "inf"); return; }
  if (number > 4294967040.0 || number < -4294967040.0) { strcpy(buf, "ovf"); return; }
  if (number < 0.0) { buf[n++] = '-'; number = -number; }
  double rounding = 0.5;
  for (uint8_t i = 0; i < digits; ++i) rounding /= 10.0;
  number += rounding;
  unsigned long int_part = (unsigned long)number;
  double remainder = number - (double)int_part;
  n += sprintf(buf + n, "%lu", int_part);
  if (digits > 0) buf[n++] = '.';
  while (digits-- > 0) {
    remainder *= 10.0;
    unsigned int toPrint = (unsigned int)(remainder);
    buf[n++] = (char)('0' + toPrint);
    remainder -= toPrint;
  }
  buf[n] = 0;
}

#define CELL_X0 0
#define CELL_X1 43
#define CELL_X2 86

static void drawCell(uint8_t x, const char *label, double value, uint8_t decimals,
                     const char *unit) {
  char num[16];
  /* label: 5x7, baseline y=9 */
  u8g2_SetFont(&u8g2, u8g2_font_5x7_tr);
  u8g2_DrawStr(&u8g2, x + 1, 9, label);
  /* value: logisoso18, baseline y=42 */
  u8g2_SetFont(&u8g2, u8g2_font_logisoso18_tr);
  print_float(value, decimals, num);
  u8g2_DrawStr(&u8g2, x + 1, 42, num);
  u8g2_uint_t adv = u8g2_GetStrWidth(&u8g2, num);
  /* unit: 5x7, baseline y=56 (own row, so value width never matters) */
  u8g2_SetFont(&u8g2, u8g2_font_5x7_tr);
  u8g2_DrawStr(&u8g2, x + 1, 56, unit);
  fprintf(stderr, "  %-5s value=\"%s\" w=%u cellx=%u value_right=%u\n",
          label, num, (unsigned)adv, (unsigned)x, (unsigned)(x + 1 + adv));
}

static void drawAll(double w, double o, double p) {
  drawCell(CELL_X0, "WATER", w, 0, "C");
  drawCell(CELL_X1, "OILT", o, 0, "C");
  drawCell(CELL_X2, "OILP", p, 1, "kg");
  /* column separators */
  u8g2_DrawVLine(&u8g2, 42, 0, 64);
  u8g2_DrawVLine(&u8g2, 85, 0, 64);
}

int main(int argc, char **argv) {
  double w = (argc > 1) ? atof(argv[1]) : 25.0;
  double o = (argc > 2) ? atof(argv[2]) : 80.0;
  double p = (argc > 3) ? atof(argv[3]) : 4.5;
  int mode = (argc > 4) ? atoi(argv[4]) : 1;
  force_empty = (mode == 2);

  u8g2_Setup_ssd1309_128x64_noname0_f(&u8g2, U8G2_R0, u8x8_byte_empty, u8x8_dummy_cb);
  u8g2_InitDisplay(&u8g2);
  u8g2_SetPowerSave(&u8g2, 0);
  u8g2_ClearBuffer(&u8g2);
  drawAll(w, o, p);
  u8g2_WriteBufferPBM(&u8g2, out_cb);
  return 0;
}
