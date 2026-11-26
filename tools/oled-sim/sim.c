/* OLED render simulator (mode 2 = value string forced empty, to reproduce the
 * field photo). Same U8g2 2.36.18 / fonts / page-buffer path as the firmware.
 *   mode 0: full buffer, one 256x64 PBM
 *   mode 1: page buffer, one PBM per page with "###PAGE n" markers
 *   mode 2: page buffer, value string forced empty
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

static void drawSensorCell(uint8_t x, const char *label, double value,
                           uint8_t decimals, const char *unit) {
  char num[32];
  u8g2_SetFont(&u8g2, u8g2_font_helvB12_tr);
  u8g2_DrawStr(&u8g2, x, 14, label);

  u8g2_SetFont(&u8g2, u8g2_font_logisoso24_tr);
  print_float(value, decimals, num);
  u8g2_DrawStr(&u8g2, x, 52, num);
  u8g2_uint_t adv = u8g2_GetStrWidth(&u8g2, num);

  u8g2_SetFont(&u8g2, u8g2_font_helvB10_tr);
  u8g2_DrawStr(&u8g2, x + adv, 52, unit);

  fprintf(stderr, "  %-5s value=\"%s\" w=%u unit@x=%u\n",
          label, num, (unsigned)adv, (unsigned)(x + adv));
}

static void drawAll(double w, double o, double p) {
  drawSensorCell(2, "Water", w, 0, "C");
  drawSensorCell(90, "OilT", o, 0, "C");
  drawSensorCell(176, "OilP", p, 1, "kg");
}

int main(int argc, char **argv) {
  double w = (argc > 1) ? atof(argv[1]) : 25.0;
  double o = (argc > 2) ? atof(argv[2]) : 80.0;
  double p = (argc > 3) ? atof(argv[3]) : 4.5;
  int mode = (argc > 4) ? atoi(argv[4]) : 0;
  force_empty = (mode == 2);

  if (mode == 0) {
    u8g2_Setup_ssd1322_nhd_256x64_f(&u8g2, U8G2_R0, u8x8_byte_empty, u8x8_dummy_cb);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    u8g2_ClearBuffer(&u8g2);
    drawAll(w, o, p);
    u8g2_WriteBufferPBM(&u8g2, out_cb);
  } else {
    u8g2_Setup_ssd1322_zjy_256x64_1(&u8g2, U8G2_R0, u8x8_byte_empty, u8x8_dummy_cb);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    u8g2_ClearDisplay(&u8g2);
    u8g2_FirstPage(&u8g2);
    do {
      drawAll(w, o, p);
      printf("###PAGE %d\n", (int)u8g2_GetBufferCurrTileRow(&u8g2));
      u8g2_WriteBufferPBM(&u8g2, out_cb);
    } while (u8g2_NextPage(&u8g2));
  }
  return 0;
}
