//
// Example showing how to draw multiple sizes of the same font
// on an epaper display (using the bb_epaper library)
//
#include <bb_epaper.h>
#include <bb_truetype.h>
#include "Roboto_Black_ttf.h"

BBEPAPER bbep(EPD295_128x296);
bb_truetype bbtt;

// Mike Rankin's C3 e-ink PCB
#define DC_PIN 3
#define BUSY_PIN 1
#define RESET_PIN 10
#define CS_PIN 7
#define MOSI_PIN 6
#define SCK_PIN 4

void DrawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint32_t color)
{
  bbep.drawLine(x1, y1, x2, y2, (uint16_t)color); // use the line function from bb_spi_lcd
}

void setup()
{
    bbep.initIO(DC_PIN, RESET_PIN, BUSY_PIN, CS_PIN, MOSI_PIN, SCK_PIN, 8000000);
    bbep.setRotation(90);
    bbep.allocBuffer();
    bbep.fillScreen(BBEP_WHITE);
//    #ifdef BOGUS
    bbtt.setTtfDrawLine(DrawLine); // pass the pointer to our drawline callback function
    bbtt.setTextBoundary(0, bbep.width(), bbep.height());
    bbtt.setTextColor(COLOR_NONE, BBEP_BLACK);
    bbtt.setTextAlignment(TEXT_ALIGN_CENTER);
    bbtt.setTtfPointer((uint8_t *)Roboto_Black, sizeof(Roboto_Black)); // use the font from FLASH
    bbtt.setCharacterSize(20);
    bbtt.textDraw(0, 20, "Test");
    bbtt.setCharacterSize(40);
    bbtt.textDraw(0, 60, "Test");
    bbtt.setCharacterSize(80);
    bbtt.textDraw(0, 120, "Test");
//    #endif
  bbep.writePlane(PLANE_DUPLICATE);
  bbep.refresh(REFRESH_FULL);
  bbep.wait();
  bbep.sleep(DEEP_SLEEP);
} /* setup() */

void loop()
{
} /* loop() */