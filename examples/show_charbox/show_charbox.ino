//
// Example sketch to display the character box coordinates for each glyph
// The character box defines the area of the character containing pixels (different from
// the x and y advance values)
//
#include <bb_spi_lcd.h>
#include <bb_truetype.h>
#include "Roboto_Black_ttf.h"

BB_SPI_LCD lcd;
bb_truetype bbtt;

const char *szTest = "12:34 abc";

void DrawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint32_t color)
{
  lcd.drawLine(x1, y1, x2, y2, (uint16_t)color); // use the line function from bb_spi_lcd
}

void setup()
{
  int i, x;
  int baseline = 90;
  ttCharBox_t cb;
  lcd.begin(DISPLAY_CYD_2USB);
  lcd.fillScreen(TFT_BLACK);
  bbtt.setTtfDrawLine(DrawLine); // pass the pointer to our drawline callback function
  bbtt.setTtfPointer((uint8_t *)Roboto_Black, sizeof(Roboto_Black)); // use the font from FLASH
  bbtt.setCharacterSize(60); // 60 pixels tall
  bbtt.setCharacterSpacing(0);
  bbtt.setTextBoundary(0, lcd.width(), lcd.height()); // clip at display boundary
  bbtt.setTextColor(COLOR_NONE, TFT_BLUE);
  bbtt.textDraw(0, baseline, szTest); // draw the text as filled blue on a black background
  i = x = 0;
  while (szTest[i]) { // step through each character and query its bitmap size and offsets
    bbtt.getCharBox(szTest[i], &cb); // draw a white rectangle with the character box coordinates
    lcd.drawRect(x+cb.xOffset, baseline+cb.yOffset, cb.width, cb.height, TFT_WHITE);
    x += cb.xAdvance;
    i++;
  }
}

void loop()
{

}