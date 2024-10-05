//
// bb_truetype
//
// Reads truetype(.ttf) files and generates bitmap output
//
//TrueType™ Reference Manual: https://developer.apple.com/fonts/TrueType-Reference-Manual/
// get info on a ttf file: https://fontdrop.info/

// MIT licencse
// original source by https://github.com/garretlab/truetype
// extended by https://github.com/k-omura/truetype_Arduino/
// lots of bugfixes and improvements by Nic.
// Completely rewritten and optimized by Larry Bank
//

#define TRUETYPE_H

#ifndef ARDUINO
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
typedef uint8_t byte;
#endif

#if !defined _SPI_H_INCLUDED && defined(ARDUINO)
#include "SPI.h"
#endif /*_SPI_H_INCLUDED*/

#if defined ESP32
#include "FS.h"
#endif /*FS_H*/

// Use this to skip either drawing the outline or the inside
#define COLOR_NONE 0x80000001

// These limits are reasonable for complex glyphs
// increase only if needed
#define MAX_POINTS 1024
#define MAX_ENDPOINTS 16

#define FLAG_ONCURVE (1 << 0)
#define FLAG_XSHORT (1 << 1)
#define FLAG_YSHORT (1 << 2)
#define FLAG_REPEAT (1 << 3)
#define FLAG_XSAME (1 << 4)
#define FLAG_YSAME (1 << 5)

#define TEXT_ALIGN_LEFT 0
#define TEXT_ALIGN_CENTER 1
#define TEXT_ALIGN_RIGHT 2

#define ROTATE_0 0
#define ROTATE_90 1
#define ROTATE_180 2
#define ROTATE_270 3

#define FILE_BUF_SIZE 256
typedef void(TTF_DRAWLINE)(int16_t _start_x, int16_t _start_y, int16_t _end_x, int16_t _end_y, uint16_t _colorCode);

typedef struct {
    char name[5];
    uint32_t checkSum;
    uint32_t offset;
    uint32_t length;
} ttTable_t;

typedef struct {
    uint32_t version;
    uint32_t revision;
    uint32_t checkSumAdjustment;
    uint32_t magicNumber;
    uint16_t flags;
    uint16_t unitsPerEm;
    char created[8];
    char modified[8];
    int16_t xMin;
    int16_t yMin;
    int16_t xMax;
    int16_t yMax;
    uint16_t macStyle;
    uint16_t lowestRecPPEM;
    int16_t fontDirectionHint;
    int16_t indexToLocFormat;
    int16_t glyphDataFormat;
} ttHeadttTable_t;

typedef struct {
    uint8_t flag;
    int16_t x;
    int16_t y;
} ttPoint_t;

typedef struct {
    int16_t numberOfContours;
    int16_t xMin;
    int16_t yMin;
    int16_t xMax;
    int16_t yMax;
    uint16_t *endPtsOfContours;
    uint16_t numberOfPoints;
    ttPoint_t *points;
} ttGlyph_t;

typedef struct {
    int16_t dx;
    int16_t dy;
    uint8_t enableScale;
    uint16_t scale_x;
    uint16_t scale_y;
} ttGlyphTransformation_t;

/* currently only support format4 cmap tables */
typedef struct {
    uint16_t version;
    uint16_t numberSubtables;
} ttCmapIndex_t;

typedef struct {
    uint16_t platformId;
    uint16_t platformSpecificId;
    uint16_t offset;
} ttCmapEncoding_t;

typedef struct {
    uint16_t format;
    uint16_t length;
    uint16_t language;
    uint16_t segCountX2;
    uint16_t searchRange;
    uint16_t entrySelector;
    uint16_t rangeShift;
    uint32_t offset;
    uint32_t endCodeOffset;
    uint32_t startCodeOffset;
    uint32_t idDeltaOffset;
    uint32_t idRangeOffsetOffset;
    uint32_t glyphIndexArrayOffset;
} ttCmapFormat4_t;

/* currently only support format0 kerning tables */
typedef struct {
    uint32_t version;  // The version number of the kerning table (0x00010000 for the current version).
    uint32_t nTables;  // The number of subtables included in the kerning table.
} ttKernHeader_t;

typedef struct {
    uint32_t length;    // The length of this subtable in bytes, including this header.
    uint16_t coverage;  // Circumstances under which this table is used. See below for description.
} ttKernSubtable_t;

typedef struct {
    uint16_t nPairs;         // The number of kerning pairs in this subtable.
    uint16_t searchRange;    // The largest power of two less than or equal to the value of nPairs, multiplied by the size in bytes of an entry in the subtable.
    uint16_t entrySelector;  // This is calculated as log2 of the largest power of two less than or equal to the value of nPairs. This value indicates how many iterations of the search loop have to be made. For example, in a list of eight items, there would be three iterations of the loop.
    uint16_t rangeShift;     // The value of nPairs minus the largest power of two less than or equal to nPairs. This is multiplied by the size in bytes of an entry in the table.
} ttKernFormat0_t;

typedef struct {
    int16_t x;
    int16_t y;
} ttCoordinate_t;

typedef struct {
    uint16_t advanceWidth;
    int16_t leftSideBearing;
} ttHMetric_t;

typedef struct {
    ttCoordinate_t *p1;
    ttCoordinate_t *p2;
    int8_t up;
} ttWindIntersect_t;

// structure holding all of the class member variables
typedef struct bbtt_tag {
#ifdef ESP32
    File file;
#endif
    uint8_t *pTTF;                   // pointer to TTF data (not from file)
    uint32_t u32TTFSize;
    uint32_t u32TTFOffset;  // current read offset into TTF data

    int iBufferedBytes;            // Number of bytes remaining in u8FileBuf
    uint8_t u8FileBuf[FILE_BUF_SIZE];  // Buffered reads from the file system
    uint32_t u32BufPosition;           // Current position in the buffer
    uint32_t iCurrentBufSize;

    TTF_DRAWLINE *pfnDrawLine;

    uint16_t charCode;
    int16_t xMin, xMax, yMin, yMax;
    uint16_t numTables;
    ttTable_t *table;
    ttHeadttTable_t headTable;
    // cmap. maps character codes to glyph indices
    ttCmapIndex_t cmapIndex;
    ttCmapEncoding_t *cmapEncoding;
    ttCmapFormat4_t cmapFormat4;
    ttGlyphTransformation_t glyphTransformation;
    uint32_t hmtxTablePos;
    // kerning.
    ttKernHeader_t kernHeader;
    ttKernSubtable_t kernSubtable;
    ttKernFormat0_t kernFormat0;
    uint32_t kernTablePos;
    int16_t ascender;
    // generate points
    ttCoordinate_t points[MAX_POINTS];
    uint16_t numPoints;
    uint16_t beginPoints[MAX_ENDPOINTS];
    uint16_t numBeginPoints;
    uint16_t endPoints[MAX_ENDPOINTS];
    uint16_t numEndPoints;

    // glyf
    ttGlyph_t glyph;
    // write user framebuffer
    uint8_t *userFrameBuffer;
    uint16_t characterSize;
    int16_t characterSpace;
    int16_t start_x;
    int16_t end_x;
    int16_t end_y;
    uint16_t displayWidth;
    uint16_t displayHeight;
    uint16_t displayWidthFrame;
    uint16_t framebufferBit;
    uint8_t stringRotation;
    uint32_t colorLine;
    uint32_t colorInside;
    uint8_t kerningOn;
    uint8_t bBigEndian;

} BBTT;

const int numTablesPos = 4;
const int tablePos = 12;

class bb_truetype {
   public:
    bb_truetype();

#ifdef ESP32
    uint8_t setTtfFile(File _file, uint8_t _checkCheckSum = 0);
#endif
    uint8_t setTtfPointer(uint8_t *pTTF, uint32_t u32Size, uint8_t _checkCheckSum = 0);
    void setTtfDrawLine(TTF_DRAWLINE *p);
    void setFramebuffer(uint16_t _framebufferWidth, uint16_t _framebufferHeight, uint16_t _framebuffer_bit, uint8_t *_framebuffer);
    void setCharacterSpacing(int16_t _characterSpace, uint8_t _kerning = 1);
    void setCharacterSize(uint16_t _characterSize);
    void setTextBoundary(uint16_t _start_x, uint16_t _end_x, uint16_t _end_y);
    void setTextColor(uint32_t _onLine, uint32_t _inside);
    void setTextRotation(uint16_t _rotation);

    uint16_t getStringWidth(const wchar_t *szwString);
    uint16_t getStringWidth(const char *szString);
#ifdef ARDUINO
    uint16_t getStringWidth(const String _string);
    void textDraw(int16_t _x, int16_t _y, const String _string);
#endif
    
    void textDraw(int16_t _x, int16_t _y, const wchar_t _character[]);
    void textDraw(int16_t _x, int16_t _y, const char _character[]);

    void end();

   private:

    BBTT _bbtt;
    uint8_t getUInt8t();
    int16_t getInt16t();
    uint16_t getUInt16t();
    uint32_t getUInt32t();

    int ttfRead(uint8_t *d, int iLen);
    void ttfSeek(uint32_t u32Offset);
    uint32_t ttfPosition(void);
    // basic
    uint32_t calculateCheckSum(uint32_t offset, uint32_t length);
    uint32_t seekToTable(const char *name);
    int readTableDirectory(int checkCheckSum);
    void readHeadTable();
    void readCoords(char _xy, uint16_t _startPoint = 0);

    // Glyph
    uint32_t getGlyphOffset(uint16_t index);
    uint16_t codeToGlyphId(uint16_t code);
    uint8_t readSimpleGlyph(uint8_t _addGlyph = 0);
    uint8_t readCompoundGlyph();

    uint8_t readCmapFormat4();
    uint8_t readCmap();

    // hmtx. metric information for the horizontal layout each of the glyphs
    uint8_t readHMetric();
    ttHMetric_t getHMetric(uint16_t _code);

    uint8_t readKern();
    int16_t getKerning(uint16_t _left_glyph, uint16_t _right_glyph);
    uint8_t readHhea();

    void generateOutline(int16_t _x, int16_t _y, uint16_t characterSize);
    void drawOutline(int16_t _x, int16_t _y, uint16_t characterSize);
    void fillGlyph(int16_t _x_min, int16_t _y_min, uint16_t characterSize);
    uint8_t readGlyph(uint16_t code, uint8_t _justSize = 0);
    void freeGlyph();

    void addLine(int16_t _x0, int16_t _y0, int16_t _x1, int16_t _y1);
    int32_t isLeft(ttCoordinate_t *_p0, ttCoordinate_t *_p1, ttCoordinate_t *_point);

#ifdef ARDUINO
    void stringToWchar(String _string, wchar_t _charctor[]);
#endif
    void drawLine(int16_t _start_x, int16_t _start_y, int16_t _end_x, int16_t _end_y, uint32_t _colorCode);
    uint8_t GetU8ByteCount(char _ch);
    bool IsU8LaterByte(char _ch);
};
