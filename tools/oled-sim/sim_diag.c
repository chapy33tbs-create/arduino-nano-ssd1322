/* Renderer for the one-shot diagnostic screens (DIAG_FIRMWARE) on the
 * production SSD1322 panel (ZJY270S0700XG31, 256x64, page buffer).
 * Mirrors drawDiag() in src/main.cpp so the layout can be checked on a PC.
 *   node run_diag.mjs <screen 0..2> > out.pbm
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "u8g2.h"

static u8g2_t u8g2;
static void out_cb(const char *s) { fputs(s, stdout); }

static void drawDiag(int screen, double water, double oil, double press,
                     double vcc, int raw_w, int raw_o, int raw_p, int freeram) {
  char buf[40];
  u8g2_FirstPage(&u8g2);
  do {
    if (screen == 0) {
      u8g2_DrawFrame(&u8g2, 0, 0, 256, 64);
      for (uint8_t y = 0; y < 64; y += 8) u8g2_DrawHLine(&u8g2, 0, y, 6);
      u8g2_DrawBox(&u8g2, 0, 62, 256, 2);

      u8g2_SetFont(&u8g2, u8g2_font_5x7_tr);
      snprintf(buf, sizeof buf, "1/3 MAP  Vcc=%.2f free=%d", vcc, freeram);
      u8g2_DrawStr(&u8g2, 10, 10, buf);

      u8g2_SetFont(&u8g2, u8g2_font_logisoso24_tr);
      u8g2_DrawStr(&u8g2, 10, 48, "0123456789");

      u8g2_SetFont(&u8g2, u8g2_font_5x7_tr);
      u8g2_DrawStr(&u8g2, 10, 58, "nan inf kg/cm2  base 48");
    } else if (screen == 1) {
      u8g2_SetFont(&u8g2, u8g2_font_5x7_tr);
      u8g2_DrawStr(&u8g2, 4, 10, "2/3 PATH  fixed value 12.3");

      u8g2_SetFont(&u8g2, u8g2_font_helvB12_tr);
      u8g2_DrawStr(&u8g2, 4, 26, "print");
      u8g2_DrawStr(&u8g2, 90, 26, "str");
      u8g2_DrawStr(&u8g2, 176, 26, "draw");

      u8g2_SetFont(&u8g2, u8g2_font_logisoso24_tr);
      u8g2_DrawStr(&u8g2, 4, 58, "12.3");
      u8g2_DrawStr(&u8g2, 90, 58, "12.3");
      u8g2_DrawStr(&u8g2, 176, 58, "12.3");
    } else {
      u8g2_SetFont(&u8g2, u8g2_font_5x7_tr);
      snprintf(buf, sizeof buf, "3/3 LIVE  Vcc=%.2f", vcc);
      u8g2_DrawStr(&u8g2, 4, 10, buf);

      u8g2_SetFont(&u8g2, u8g2_font_helvB12_tr);
      snprintf(buf, sizeof buf, "W=%.0f T=%.0f P=%.1f", water, oil, press);
      u8g2_DrawStr(&u8g2, 4, 26, buf);

      u8g2_SetFont(&u8g2, u8g2_font_5x7_tr);
      snprintf(buf, sizeof buf, "raw %d,%d,%d", raw_w, raw_o, raw_p);
      u8g2_DrawStr(&u8g2, 4, 36, buf);

      u8g2_SetFont(&u8g2, u8g2_font_logisoso24_tr);
      snprintf(buf, sizeof buf, "%.0f", water);
      u8g2_DrawStr(&u8g2, 4, 62, buf);

      u8g2_SetFont(&u8g2, u8g2_font_5x7_tr);
      u8g2_DrawStr(&u8g2, 96, 62, "<- same W, logisoso24");
    }
    printf("###PAGE %d\n", (int)u8g2_GetBufferCurrTileRow(&u8g2));
    u8g2_WriteBufferPBM(&u8g2, out_cb);
  } while (u8g2_NextPage(&u8g2));
}

int main(int argc, char **argv) {
  int screen = (argc > 1) ? atoi(argv[1]) : 0;
  u8g2_Setup_ssd1322_zjy_256x64_1(&u8g2, U8G2_R0, u8x8_byte_empty, u8x8_dummy_cb);
  u8g2_InitDisplay(&u8g2);
  u8g2_SetPowerSave(&u8g2, 0);
  u8g2_ClearDisplay(&u8g2);
  drawDiag(screen, 8.0, 2.0, 3.9, 4.95, 374, 351, 380, 521);
  return 0;
}
