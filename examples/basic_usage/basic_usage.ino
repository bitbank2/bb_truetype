//
// bb_truetype
// library basic usage example
//
// This example provides a static pointer to a TTF file embedded in FLASH (compiled into the program)
// and uses the DrawLine() callback to avoid allocating a bitmap to draw into.
// The characters are formed by repeatedly calling the DrawLine() function
//
#include <bb_truetype.h>
#include <bb_spi_lcd.h>
#include "Roboto_Black_ttf.h"

BB_SPI_LCD lcd;
bb_truetype bbtt;

void DrawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
  lcd.drawLine(x1, y1, x2, y2, color); // use the line function from bb_spi_lcd
}

void setup()
{
  lcd.begin(DISPLAY_CYD_2USB);
  lcd.fillScreen(TFT_BLACK);
  bbtt.setTtfDrawLine(DrawLine); // pass the pointer to our drawline callback function
  bbtt.setTtfPointer((uint8_t *)Roboto_Black, sizeof(Roboto_Black)); // use the font from FLASH
  bbtt.setCharacterSize(48);
  bbtt.setCharacterSpacing(0);
  bbtt.setTextBoundary(0, lcd.width(), lcd.height());
  bbtt.setTextColor(TFT_BLUE, COLOR_NONE); // no fill color, just outline
  bbtt.textDraw(0, 80, "Hello");
  bbtt.setTextColor(COLOR_NONE, TFT_GREEN); // only fill, no outline
  bbtt.textDraw(0, 180, "World!");
} /* setup() */

void loop()
{
} /* loop() */

