//
// bb_truetype
//
// An imperfect, but fast TrueType renderer for constrained systems
// written by Larry Bank
// Copyright (c) 2024 BitBank Software, Inc.
//
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void bbttInit(BBTT *pBBTT)
{
    pBBTT->numPoints = pBBTT->numBeginPoints = pBBTT->numEndPoints = 0;
    pBBTT->bBigEndian = 0;
    pBBTT->stringRotation = 0;
    pBBTT->characterSpace = 0;
    pBBTT->kerningOn = 1;
    pBBTT->kernTablePos = 0;
    pBBTT->ascender = 0;
    pBBTT->iBufferedBytes = 0;
    pBBTT->hmtxTablePos = 0;
    pBBTT->u32TTFSize = pBBTT->u32TTFOffset = 0;
    pBBTT->pTTF = NULL;
    pBBTT->pfnDrawLine = NULL;
    pBBTT->iCurrentBufSize = 0;
} /* bbttInit() */

void bbttSetRotation(BBTT *pBBTT, uint16_t _rotation)
{
    switch (_rotation) {
        case ROTATE_90:
        case 90:
            _rotation = 1;
            break;
        case ROTATE_180:
        case 180:
            _rotation = 2;
            break;
        case ROTATE_270:
        case 270:
            _rotation = 3;
            break;
        default:
            _rotation = 0;
            break;
    }
    pBBTT->stringRotation = _rotation;
} /* bbttSetRotation() */

void bbttSetTextColor(BBTT *pBBTT, uint16_t _onLine, uint16_t _inside)
{
    pBBTT->colorLine = _onLine;
    pBBTT->colorInside = _inside;
} /* bbttSetTextColor() */

int bbttRead(BBTT *pBBTT, uint8_t *d, int iLen) {
    if (!pBBTT->pTTF) {
        //return file.read(d, iLen);
        int totalBytesRead = 0;
#ifdef ESP32
        while (iLen > 0) {
            if (iBufferedBytes == 0) {
                iBufferedBytes = file.read(u8FileBuf, FILE_BUF_SIZE);
                iCurrentBufSize = iBufferedBytes;
                if (iBufferedBytes <= 0) break;
                u32BufPosition = 0;
            }

            int bytesToCopy = min(iLen, iBufferedBytes);
            memcpy(d, u8FileBuf + u32BufPosition, bytesToCopy);

            d += bytesToCopy;
            iLen -= bytesToCopy;
            iBufferedBytes -= bytesToCopy;
            u32BufPosition += bytesToCopy;
            totalBytesRead += bytesToCopy;
        }
#endif // ESP32
        return totalBytesRead;
    } else {

        if (pBBTT->u32TTFOffset + iLen > pBBTT->u32TTFSize) {
            iLen = pBBTT->u32TTFSize - pBBTT->u32TTFOffset;
        }
        memcpy(d, &pBBTT->pTTF[pBBTT->u32TTFOffset], iLen);
        pBBTT->u32TTFOffset += iLen;
        return iLen;
    }
    return 0;
} /* bbttRead() */

uint8_t bbttGetUInt8t(BBTT *pBBTT)
{
    uint8_t x;
    bbttRead(pBBTT, &x, 1);
    return x;
} /* bbttGetUInt8t */

uint16_t bbttGetUInt16t(BBTT *pBBTT)
{
    uint16_t x;
    bbttRead(pBBTT, (uint8_t *)&x, 2);
    return __builtin_bswap16(x);
} /* bbttGetUInt16t */

int16_t bbttGetInt16t(BBTT *pBBTT)
{
    int16_t x;
    bbttRead(pBBTT, (uint8_t *)&x, 2);
    return __builtin_bswap16(x);
} /* bbttGetInt16t */

uint32_t bbttGetUInt32t(BBTT *pBBTT) {
    uint32_t x;
    bbttRead(pBBTT, (uint8_t *)&x, 4);
    return __builtin_bswap32(x);
} /* bbttGetUInt32t() */

void bbttSeek(BBTT *pBBTT, uint32_t u32Offset) {
    if (!pBBTT->pTTF) {
        /*
        TODO/FIXME: for some reason this doesn't work.
        If a seek position is within the current loaded buffer, it should just change the buffer position

        if (u32Offset >= file.position() - iCurrentBufSize && u32Offset < file.position()) {
            u32BufPosition = u32Offset - (file.position() - iCurrentBufSize);
            iBufferedBytes = file.position() - u32Offset;
        } else {
        */
#ifdef ESP32
            file.seek(u32Offset);
#endif
        pBBTT->iBufferedBytes = 0;
        // }
    } else {
        if (u32Offset > pBBTT->u32TTFSize) {
            u32Offset = pBBTT->u32TTFSize;
        }
        pBBTT->u32TTFOffset = u32Offset;
    }
} /* bbttSeek() */

/* calculate checksum */
uint32_t bbttCalculateCheckSum(BBTT *pBBTT, uint32_t offset, uint32_t length) {
    uint32_t checksum = 0L;

    length = (length + 3) / 4;
    bbttSeek(pBBTT, offset);

    while (length-- > 0) {
        checksum += bbttGetUInt32t(pBBTT);
    }
    return checksum;
}

uint32_t bbttPosition(BBTT *pBBTT) {
    if (!pBBTT->pTTF) {
#ifdef ESP32
        return file.position() - iBufferedBytes;
#else
        return 0;
#endif
    } else {
        return pBBTT->u32TTFOffset;
    }
} /* ttfPosition() */

/* convert character code to glyph id */
uint16_t bbttCodeToGlyphId(BBTT *pBBTT, uint16_t _code)
{
    uint16_t start, end, idRangeOffset;
    int16_t idDelta;
    uint8_t found = 0;
    uint16_t offset, glyphId = 0;

    for (int i = 0; i < pBBTT->cmapFormat4.segCountX2 / 2; i++) {
        bbttSeek(pBBTT, pBBTT->cmapFormat4.endCodeOffset + 2 * i);
        end = bbttGetUInt16t(pBBTT);
        if (_code <= end) {
            bbttSeek(pBBTT, pBBTT->cmapFormat4.startCodeOffset + 2 * i);
            start = bbttGetUInt16t(pBBTT);
            if (_code >= start) {
                bbttSeek(pBBTT, pBBTT->cmapFormat4.idDeltaOffset + 2 * i);
                idDelta = bbttGetInt16t(pBBTT);
                bbttSeek(pBBTT, pBBTT->cmapFormat4.idRangeOffsetOffset + 2 * i);
                idRangeOffset = bbttGetUInt16t(pBBTT);
                if (idRangeOffset == 0) {
                    glyphId = (idDelta + _code) % 65536;
                } else {
                    offset = (idRangeOffset / 2 + i + _code - start - pBBTT->cmapFormat4.segCountX2 / 2) * 2;
                    bbttSeek(pBBTT, pBBTT->cmapFormat4.glyphIndexArrayOffset + offset);
                    glyphId = bbttGetUInt16t(pBBTT);
                }

                found = 1;
                break;
            }
        }
    }
    if (!found) {
        return 0;
    }
    return glyphId;
} /* bbttCodeToGlyphId() */

/* get glyph offset */
uint32_t bbttGetGlyphOffset(BBTT *pBBTT, uint16_t index) {
    uint32_t offset = 0;

    for (int i = 0; i < pBBTT->numTables; i++) {
        if (strcmp(pBBTT->table[i].name, "loca") == 0) {
            if (pBBTT->headTable.indexToLocFormat == 1) {
                bbttSeek(pBBTT, pBBTT->table[i].offset + index * 4);
                offset = bbttGetUInt32t(pBBTT);
            } else {
                bbttSeek(pBBTT, pBBTT->table[i].offset + index * 2);
                offset = bbttGetUInt16t(pBBTT) * 2;
            }
            break;
        }
    }

    for (int i = 0; i < pBBTT->numTables; i++) {
        if (strcmp(pBBTT->table[i].name, "glyf") == 0) {
            return (offset + pBBTT->table[i].offset);
        }
    }

    return 0;
}

/* read coords */
void bbttReadCoords(BBTT *pBBTT, char _xy, uint16_t _startPoint) {
    int16_t value = 0;
    uint8_t shortFlag, sameFlag;

    if (_xy == 'x') {
        shortFlag = FLAG_XSHORT;
        sameFlag = FLAG_XSAME;
    } else {
        shortFlag = FLAG_YSHORT;
        sameFlag = FLAG_YSAME;
    }

    for (uint16_t i = _startPoint; i < pBBTT->glyph.numberOfPoints; i++) {
        if (pBBTT->glyph.points[i].flag & shortFlag) {
            if (pBBTT->glyph.points[i].flag & sameFlag) {
                value += bbttGetUInt8t(pBBTT);
            } else {
                value -= bbttGetUInt8t(pBBTT);
            }
        } else if (~pBBTT->glyph.points[i].flag & sameFlag) {
            value += bbttGetUInt16t(pBBTT);
        }

        if (_xy == 'x') {
            if (pBBTT->glyphTransformation.enableScale) {
                pBBTT->glyph.points[i].x = value + pBBTT->glyphTransformation.dx;
            } else {
                pBBTT->glyph.points[i].x = value + pBBTT->glyphTransformation.dx;
            }
        } else {
            if (pBBTT->glyphTransformation.enableScale) {
                pBBTT->glyph.points[i].y = value + pBBTT->glyphTransformation.dy;
            } else {
                pBBTT->glyph.points[i].y = value + pBBTT->glyphTransformation.dy;
            }
        }
    }
}

/* read simple glyph */
uint8_t bbttReadSimpleGlyph(BBTT *pBBTT, uint8_t _addGlyph) {
    uint8_t repeatCount;
    uint8_t flag;
    static uint16_t counterContours;
    static uint16_t counterPoints;

    if (pBBTT->glyph.numberOfContours <= 0) {
        return 0;
    }

    if (!_addGlyph) {
        counterContours = 0;
        counterPoints = 0;
    }

    if (_addGlyph) {
        pBBTT->glyph.endPtsOfContours = (uint16_t *)realloc(pBBTT->glyph.endPtsOfContours, (sizeof(uint16_t) * pBBTT->glyph.numberOfContours));
    } else {
        pBBTT->glyph.endPtsOfContours = (uint16_t *)malloc((sizeof(uint16_t) * pBBTT->glyph.numberOfContours));
    }

    for (uint16_t i = counterContours; i < pBBTT->glyph.numberOfContours; i++) {
        pBBTT->glyph.endPtsOfContours[i] = counterPoints + bbttGetUInt16t(pBBTT);
    }

    bbttSeek(pBBTT, bbttGetUInt16t(pBBTT) + bbttPosition(pBBTT));

    for (uint16_t i = counterContours; i < pBBTT->glyph.numberOfContours; i++) {
        if (pBBTT->glyph.endPtsOfContours[i] > pBBTT->glyph.numberOfPoints) {
            pBBTT->glyph.numberOfPoints = pBBTT->glyph.endPtsOfContours[i];
        }
    }
    pBBTT->glyph.numberOfPoints++;

    if (_addGlyph) {
        pBBTT->glyph.points = (ttPoint_t *)realloc(pBBTT->glyph.points, sizeof(ttPoint_t) * (pBBTT->glyph.numberOfPoints + pBBTT->glyph.numberOfContours));
    } else {
        pBBTT->glyph.points = (ttPoint_t *)malloc(sizeof(ttPoint_t) * (pBBTT->glyph.numberOfPoints + pBBTT->glyph.numberOfContours));
    }

    for (uint16_t i = counterPoints; i < pBBTT->glyph.numberOfPoints; i++) {
        flag = bbttGetUInt8t(pBBTT);
        pBBTT->glyph.points[i].flag = flag;
        if (flag & FLAG_REPEAT) {
            repeatCount = bbttGetUInt8t(pBBTT);
            while (repeatCount--) {
                pBBTT->glyph.points[++i].flag = flag;
            }
        }
    }

    bbttReadCoords(pBBTT, 'x', counterPoints);
    bbttReadCoords(pBBTT, 'y', counterPoints);

    counterContours = pBBTT->glyph.numberOfContours;
    counterPoints = pBBTT->glyph.numberOfPoints;

    return 1;
}

/* read Compound glyph */
uint8_t bbttReadCompoundGlyph(BBTT *pBBTT) {
    uint16_t glyphIndex;
    uint16_t flags;
    uint8_t numberOfGlyphs = 0;
    uint32_t offset;
    int32_t arg1, arg2;

    pBBTT->glyph.numberOfContours = 0;

    do {
        flags = bbttGetUInt16t(pBBTT);
        glyphIndex = bbttGetUInt16t(pBBTT);

        pBBTT->glyphTransformation.enableScale = (flags & 0b00000001000) ? (1) : (0);

        if (flags & 0b00000000001) {
            arg1 = bbttGetInt16t(pBBTT);
            arg2 = bbttGetInt16t(pBBTT);
        } else {
            arg1 = bbttGetUInt8t(pBBTT);
            arg2 = bbttGetUInt8t(pBBTT);
        }

        if (flags & 0b00000000010) {
            pBBTT->glyphTransformation.dx = arg1;
            pBBTT->glyphTransformation.dy = arg2;
        }

        if (flags & 0b01000000000) {
            pBBTT->charCode = glyphIndex;
        }

        offset = bbttPosition(pBBTT);

        uint32_t glyphOffset = bbttGetGlyphOffset(pBBTT, glyphIndex);
        bbttSeek(pBBTT, glyphOffset);
        pBBTT->glyph.numberOfContours += bbttGetInt16t(pBBTT);
        bbttSeek(pBBTT, glyphOffset + 10);

        if (numberOfGlyphs == 0) {
            bbttReadSimpleGlyph(pBBTT, 0);
        } else {
            bbttReadSimpleGlyph(pBBTT, 1);
        }
        bbttSeek(pBBTT, offset);

        numberOfGlyphs++;
        pBBTT->glyphTransformation = {0, 0, 0, 1, 1};  // init
    } while (flags & 0b00000100000);

    return 1;
}

/* read glyph */
uint8_t bbttReadGlyph(BBTT *pBBTT, uint16_t _code, uint8_t _justSize) {
    uint32_t offset = bbttGetGlyphOffset(pBBTT, _code);
    bbttSeek(pBBTT, offset);
    pBBTT->glyph.numberOfContours = bbttGetInt16t(pBBTT);
    pBBTT->glyph.numberOfPoints = 0;
    pBBTT->glyph.xMin = bbttGetInt16t(pBBTT);
    pBBTT->glyph.yMin = bbttGetInt16t(pBBTT);
    pBBTT->glyph.xMax = bbttGetInt16t(pBBTT);
    pBBTT->glyph.yMax = bbttGetInt16t(pBBTT);

    pBBTT->glyphTransformation = {0, 0, 0, 1, 1};  // init

    if (_justSize) {
        return 0;
    }

    if (pBBTT->glyph.numberOfContours >= 0) {
        return bbttReadSimpleGlyph(pBBTT, 0);
    } else {
        return bbttReadCompoundGlyph(pBBTT);
    }
    return 0;
}

/* seek to the first position of the specified table name */
uint32_t bbttSeekToTable(BBTT *pBBTT, const char *name) {
    for (uint32_t i = 0; i < pBBTT->numTables; i++) {
        if (strcmp(pBBTT->table[i].name, name) == 0) {
            bbttSeek(pBBTT, pBBTT->table[i].offset);
            return pBBTT->table[i].offset;
        }
    }
    return 0;
}

uint8_t bbttReadHhea(BBTT *pBBTT) {
    if (bbttSeekToTable(pBBTT, "hhea") == 0) {
        pBBTT->ascender = pBBTT->yMax;
        return 0;
    }
    bbttGetUInt32t(pBBTT);
    pBBTT->ascender = bbttGetInt16t(pBBTT);
    return 1;
}

// hmtx. metric information for the horizontal layout each of the glyphs
uint8_t bbttReadHMetric(BBTT *pBBTT) {
    if (bbttSeekToTable(pBBTT, "hmtx") == 0) {
        return 0;
    }
    pBBTT->hmtxTablePos = bbttPosition(pBBTT);
    return 1;
}

ttHMetric_t bbttGetHMetric(BBTT *pBBTT, uint16_t _code) {
    ttHMetric_t result;
    result.advanceWidth = 0;

    bbttSeek(pBBTT, pBBTT->hmtxTablePos + (_code * 4));
    result.advanceWidth = bbttGetUInt16t(pBBTT);
    result.leftSideBearing = bbttGetInt16t(pBBTT);

    result.advanceWidth = (result.advanceWidth * pBBTT->characterSize) / pBBTT->headTable.unitsPerEm;
    result.leftSideBearing = (result.leftSideBearing * pBBTT->characterSize) / pBBTT->headTable.unitsPerEm;
    return result;
}

uint16_t bbttGetStringWidthW(BBTT *pBBTT, const wchar_t *szwString)
{
    uint16_t prev_code = 0;
    uint16_t c = 0;
    uint16_t output = 0;

    while (szwString[c] != '\0') {
        // space (half-width, full-width)
        if ((szwString[c] == ' ') || (szwString[c] == L'　')) {
            prev_code = 0;
            output += pBBTT->characterSize / 4;
            c++;
            continue;
        }
        uint16_t code = bbttCodeToGlyphId(pBBTT, szwString[c]);
        bbttReadGlyph(pBBTT, code, 1);

        output += pBBTT->characterSpace;
#ifdef ENABLEKERNING
        if (prev_code != 0 && pBBTT->kerningOn) {
            int16_t kern = getKerning(prev_code, code);  // space between charctor
            output += (kern * (int16_t)pBBTT->characterSize) / pBBTT->headTable.unitsPerEm;
        }
#endif
        prev_code = code;

        ttHMetric_t hMetric = bbttGetHMetric(pBBTT, code);
        output += hMetric.advanceWidth;
        c++;
    }

    return output;

} /* bbttGetStringWidthW() */

uint16_t bbttGetStringWidth(BBTT *pBBTT, const char *szString)
{
    wchar_t *szwString;
    uint16_t length = 0;
    uint16_t output = 0;
    while (szString[length] != '\0') {
        length++;
    }
    szwString = (wchar_t *)malloc(sizeof(wchar_t) * (length + 1));
    for (uint16_t i = 0; i <= length; i++) {
        szwString[i] = szString[i];
    }
    output = bbttGetStringWidthW(pBBTT, szwString);
    free(szwString);
    return output;

} /* bbttGetStringWidth() */

void bbttSetTextBoundary(BBTT *pBBTT, uint16_t _start_x, uint16_t _end_x, uint16_t _end_y)
{
    pBBTT->start_x = _start_x;
    pBBTT->end_x = _end_x;
    pBBTT->end_y = _end_y;
}

void bbttDrawPixel(BBTT *pBBTT, uint16_t _x, uint16_t _y, uint16_t _colorCode)
{
    uint8_t *buf_ptr;

    // limit to boundary co-ordinates the boundary is always in the same orientation as the string not the buffer
    if ((_x < pBBTT->start_x) || (_x >= pBBTT->end_x) || (_y >= pBBTT->end_y)) {
        return;
    }

    // Rotate co-ordinates relative to the buffer
    uint16_t temp = _x;
    switch (pBBTT->stringRotation) {
        case ROTATE_270:
            _x = _y;
            _y = pBBTT->displayHeight - 1 - temp;
            break;
        case ROTATE_180:
            _x = pBBTT->displayWidth - 1 - _x;
            _y = pBBTT->displayHeight - 1 - _y;
            break;
        case ROTATE_90:
            _x = pBBTT->displayWidth - 1 - _y;
            _y = temp;
            break;
        case 0:
        default:
            break;
    }

    // out of range
    if ((_x < 0) || ((uint16_t)_x >= pBBTT->displayWidth) || ((uint16_t)_y >= pBBTT->displayHeight) || (_y < 0)) {
        return;
    }

    switch (pBBTT->framebufferBit) {
        case 16:  // 16bit horizontal
        {
            uint16_t *p = (uint16_t *)&pBBTT->userFrameBuffer[(uint16_t)_x * 2 + (uint16_t)_y * pBBTT->displayWidthFrame];
            if (pBBTT->bBigEndian) {
                _colorCode = (_colorCode >> 8) | (_colorCode << 8);
            }
            *p = _colorCode;
        } break;
        case 8:  // 8bit Horizontal
        {
            pBBTT->userFrameBuffer[(uint16_t)_x + (uint16_t)_y * pBBTT->displayWidthFrame] = (uint8_t)_colorCode;
        } break;
        case 4:  // 4bit Horizontal
        {
            buf_ptr = &pBBTT->userFrameBuffer[((uint16_t)_x / 2) + (uint16_t)_y * pBBTT->displayWidthFrame];
            _colorCode = _colorCode & 0b00001111;

            if ((uint16_t)_x & 1) {
                *buf_ptr = (*buf_ptr & 0b11110000) + _colorCode;
            } else {
                *buf_ptr = (*buf_ptr & 0b00001111) + (_colorCode << 4);
            }
        } break;
        case 1:  // 1bit Horizontal
        default: {
            buf_ptr = &pBBTT->userFrameBuffer[((uint16_t)_x / 8) + (uint16_t)_y * pBBTT->displayWidthFrame];
            uint8_t bitMask = 0b10000000 >> ((uint16_t)_x % 8);
            uint8_t bit = (_colorCode) ? (bitMask) : (0b00000000);
            *buf_ptr = (*buf_ptr & ~bitMask) + bit;
        } break;
    }
} /* bbttDrawPixel() */

void bbttDrawLine(BBTT *pBBTT, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t _colorCode)
{
    int temp;
    int dx = x2 - x1;
    int dy = y2 - y1;
    int error;
    int xinc, yinc;
    
    if(abs(dx) > abs(dy)) {
        // X major case
        if (x2 < x1) {
            dx = -dx;
            temp = x1;
            x1 = x2;
            x2 = temp;
            temp = y1;
            y1 = y2;
            y2 = temp;
        }
        dy = (y2 - y1);
        error = dx >> 1;
        yinc = 1;
        if (dy < 0) {
            dy = -dy;
            yinc = -1;
        }
        for(; x1 <= x2; x1++) {
            bbttDrawPixel(pBBTT, x1, y1, _colorCode);
            error -= dy;
            if (error < 0) { // y needs to change, write existing pixels
                error += dx;
                y1 += yinc;
            }
        } // for x1
    } else {
        // Y major case
        if(y1 > y2) {
            dy = -dy;
            temp = x1;
            x1 = x2;
            x2 = temp;
            temp = y1;
            y1 = y2;
            y2 = temp;
        }
        dx = (x2 - x1);
        error = dy >> 1;
        xinc = 1;
        if (dx < 0) {
            dx = -dx;
            xinc = -1;
        }
        for(; y1 <= y2; y1++) {
            bbttDrawPixel(pBBTT, x1, y1, _colorCode);
            error -= dx;
            if (error < 0) { // x needs to change, write any pixels we traversed
                error += dy;
                x1 += xinc;
            }
        } // for y
    } // y major case
} /* bbttDrawLine() */

void bbttDrawOutline(BBTT *pBBTT, int16_t _x, int16_t _y, uint16_t characterSize)
{
    uint16_t p2Num, epCounter = 0;
    
    for (uint16_t i = 0; i < pBBTT->numPoints; i++) {
        // Wrap?
        if (i == pBBTT->endPoints[epCounter]) {
            p2Num = pBBTT->beginPoints[epCounter];
            epCounter++;
        } else {
            p2Num = i + 1;
        }
        if (pBBTT->pfnDrawLine) {
            (*pBBTT->pfnDrawLine)(pBBTT->points[i].x, pBBTT->points[i].y, pBBTT->points[p2Num].x, pBBTT->points[p2Num].y, pBBTT->colorLine);
        } else {
            bbttDrawLine(pBBTT, pBBTT->points[i].x, pBBTT->points[i].y, pBBTT->points[p2Num].x, pBBTT->points[p2Num].y, pBBTT->colorLine);
        }
    }
} /* drawOutline() */

void bbttAddLine(BBTT *pBBTT, int16_t _x0, int16_t _y0, int16_t _x1, int16_t _y1) {
    if (pBBTT->numPoints == 0) {
        pBBTT->points[pBBTT->numPoints].x = _x0;
        pBBTT->points[pBBTT->numPoints++].y = _y0;
        pBBTT->beginPoints[pBBTT->numBeginPoints++] = 0;
    }
    pBBTT->points[pBBTT->numPoints].x = _x1;
    pBBTT->points[pBBTT->numPoints++].y = _y1;
}

// generate Bitmap
void bbttGenerateOutline(BBTT *pBBTT, int16_t _x, int16_t _y, uint16_t characterSize) {
    pBBTT->numPoints = 0;
    pBBTT->numBeginPoints = 0;
    pBBTT->numEndPoints = 0;

    float x0, y0, x1, y1;

    uint16_t j = 0;

    for (uint16_t i = 0; i < pBBTT->glyph.numberOfContours; i++) {
        uint8_t firstPointOfContour = j;
        uint8_t lastPointOfContour = pBBTT->glyph.endPtsOfContours[i];

        // Rotate to on-curve the first point
        uint16_t numberOfRotations = 0;
        while ((firstPointOfContour + numberOfRotations) <= lastPointOfContour) {
            if (pBBTT->glyph.points[(firstPointOfContour + numberOfRotations)].flag & FLAG_ONCURVE) {
                break;
            }
            numberOfRotations++;
        }
        if ((j + numberOfRotations) <= lastPointOfContour) {
            for (uint16_t ii = 0; ii < numberOfRotations; ii++) {
                ttPoint_t tmp = pBBTT->glyph.points[firstPointOfContour];
                for (uint16_t jj = firstPointOfContour; jj < lastPointOfContour; jj++) {
                    pBBTT->glyph.points[jj] = pBBTT->glyph.points[jj + 1];
                }
                pBBTT->glyph.points[lastPointOfContour] = tmp;
            }
        }

        ttCoordinate_t pointsOfCurve[6];
        pointsOfCurve[0].x = pBBTT->glyph.points[j].x;
        pointsOfCurve[0].y = pBBTT->glyph.points[j].y;

        while (j <= lastPointOfContour) {

            uint16_t searchPoint = (j == lastPointOfContour) ? (firstPointOfContour) : (j + 1);
            
            pointsOfCurve[1].x = pBBTT->glyph.points[searchPoint].x;
            pointsOfCurve[1].y = pBBTT->glyph.points[searchPoint].y;

            if (pBBTT->glyph.points[searchPoint].flag & FLAG_ONCURVE) {

                bbttAddLine(pBBTT, pointsOfCurve[0].x * pBBTT->characterSize / pBBTT->headTable.unitsPerEm + _x,
                        (pBBTT->ascender - pointsOfCurve[0].y) * pBBTT->characterSize / pBBTT->headTable.unitsPerEm + _y,
                        pointsOfCurve[1].x * pBBTT->characterSize / pBBTT->headTable.unitsPerEm + _x,
                        (pBBTT->ascender - pointsOfCurve[1].y) * pBBTT->characterSize / pBBTT->headTable.unitsPerEm + _y);

                pointsOfCurve[0] = pointsOfCurve[1];
                j += 1;

            } else {

                searchPoint = (searchPoint == lastPointOfContour) ? (firstPointOfContour) : (searchPoint + 1);

                if (pBBTT->glyph.points[searchPoint].flag & FLAG_ONCURVE) {
                    pointsOfCurve[2].x = pBBTT->glyph.points[searchPoint].x;
                    pointsOfCurve[2].y = pBBTT->glyph.points[searchPoint].y;
                    j += 2;
                } else {
                    pointsOfCurve[2].x = (pointsOfCurve[1].x + pBBTT->glyph.points[searchPoint].x) / 2;
                    pointsOfCurve[2].y = (pointsOfCurve[1].y + pBBTT->glyph.points[searchPoint].y) / 2;
                    j += 1;
                }

                x0 = pointsOfCurve[0].x;
                y0 = pointsOfCurve[0].y;

                for (int step = 0; step <= 9; step += 1) {
                    float t = (float)step / 9.0;
                    x1 = (1.0 - t) * (1.0 - t) * pointsOfCurve[0].x + 2.0 * t * (1.0 - t) * pointsOfCurve[1].x + t * t * pointsOfCurve[2].x;
                    y1 = (1.0 - t) * (1.0 - t) * pointsOfCurve[0].y + 2.0 * t * (1.0 - t) * pointsOfCurve[1].y + t * t * pointsOfCurve[2].y;

                    bbttAddLine(pBBTT, x0 * pBBTT->characterSize / pBBTT->headTable.unitsPerEm + _x, (pBBTT->ascender - y0) * pBBTT->characterSize / pBBTT->headTable.unitsPerEm + _y,
                            x1 * pBBTT->characterSize / pBBTT->headTable.unitsPerEm + _x, (pBBTT->ascender - y1) * pBBTT->characterSize / pBBTT->headTable.unitsPerEm + _y);

                    x0 = x1;
                    y0 = y1;
                }

                pointsOfCurve[0] = pointsOfCurve[2];

            }
        }
        pBBTT->endPoints[pBBTT->numEndPoints++] = pBBTT->numPoints - 1;
        pBBTT->beginPoints[pBBTT->numBeginPoints++] = pBBTT->numPoints;
    }
    return;
}

int32_t bbttIsLeft(ttCoordinate_t *_p0, ttCoordinate_t *_p1, ttCoordinate_t *_point) {
    return ((_p1->x - _p0->x) * (_point->y - _p0->y) - (_point->x - _p0->x) * (_p1->y - _p0->y));
}

void bbttFillGlyph(BBTT *pBBTT, int16_t _x_min, int16_t _y_min, uint16_t characterSize) {
    ttWindIntersect_t pointsToFill[16];
    int16_t ys = round((float)(pBBTT->ascender - pBBTT->glyph.yMax) * (float)pBBTT->characterSize / (float)pBBTT->headTable.unitsPerEm + _y_min);
    int16_t ye = round((float)(pBBTT->ascender - pBBTT->glyph.yMin) * (float)pBBTT->characterSize / (float)pBBTT->headTable.unitsPerEm + _y_min);
    int16_t xs = _x_min + round((float)pBBTT->glyph.xMin * (float)pBBTT->characterSize / (float)pBBTT->headTable.unitsPerEm);
    int16_t xe = _x_min + round((float)pBBTT->glyph.xMax * (float)pBBTT->characterSize / (float)pBBTT->headTable.unitsPerEm);
    for (int16_t y = ys; y < ye; y++) {
        ttCoordinate_t *p1, *p2;
        ttCoordinate_t point;
        int16_t p1_y, p2_y;
        point.y = y;

        uint16_t intersectPointsNum = 0;
        uint16_t epCounter = 0;
        uint16_t p2Num = 0;

        for (uint16_t i = 0; i < pBBTT->numPoints; i++) {
            p1_y = pBBTT->points[i].y;
            // Wrap?
            if (i == pBBTT->endPoints[epCounter]) {
                p2Num = pBBTT->beginPoints[epCounter];
                epCounter++;
            } else {
                p2Num = i + 1;
            }
            p2_y = pBBTT->points[p2Num].y;

            if (p1_y <= y) {
                if (p2_y > y) {
                    // Have a valid up intersect
                    pointsToFill[intersectPointsNum].p1 = &pBBTT->points[i];
                    pointsToFill[intersectPointsNum].p2 = &pBBTT->points[p2Num];
                    pointsToFill[intersectPointsNum].up = 1;
                    intersectPointsNum++;
                }
            } else {
                // start y > point.y (no test needed)
                if (p2_y <= y) {
                    // Have a valid down intersect
                    pointsToFill[intersectPointsNum].p1 = &pBBTT->points[i];
                    pointsToFill[intersectPointsNum].p2 = &pBBTT->points[p2Num];
                    pointsToFill[intersectPointsNum].up = 0;
                    intersectPointsNum++;
                }
            }
        }
        int16_t iStartPoint = -1; // start of each horizontal line segment
        for (int16_t x = xs; x < xe; x++) {
            int16_t windingNumber = 0;
            point.x = x;

            for (uint16_t i = 0; i < intersectPointsNum; i++) {
                p1 = pointsToFill[i].p1;
                p2 = pointsToFill[i].p2;

                if (pointsToFill[i].up == 1) {
                    if (bbttIsLeft(p1, p2, &point) > 0) {
                        windingNumber++;
                    }
                } else {
                    if (bbttIsLeft(p1, p2, &point) < 0) {
                        windingNumber--;
                    }
                }
            }

            if (windingNumber != 0) {
              //  addPixel(x, y, colorInside);
                if (iStartPoint == -1) {
                    iStartPoint = x; // start of a new line?
                }
            } else { // finishing a line segment?
                if (iStartPoint >= 0) {
                    if (pBBTT->pfnDrawLine) {
                        (*pBBTT->pfnDrawLine)(iStartPoint, y, x-1, y, pBBTT->colorInside);
                    } else {
                        bbttDrawLine(pBBTT, iStartPoint, y, x-1, y, pBBTT->colorInside);
                    }
                    iStartPoint = -1; // reset for next time
                }
            }
        } // for x
        if (iStartPoint >= 0) {
            if (pBBTT->pfnDrawLine) {
                (*pBBTT->pfnDrawLine)(iStartPoint, y, xe-1, y, pBBTT->colorInside);
            } else {
                bbttDrawLine(pBBTT, iStartPoint, y, xe-1, y, pBBTT->colorInside);
            }
        }
    }
}

/* free glyph */
void bbttFreeGlyph(BBTT *pBBTT)
{
    if (pBBTT->glyph.points != nullptr) free(pBBTT->glyph.points);
    if (pBBTT->glyph.endPtsOfContours != nullptr) free(pBBTT->glyph.endPtsOfContours);
    pBBTT->glyph.points = nullptr;
    pBBTT->glyph.endPtsOfContours = nullptr;
    pBBTT->glyph.numberOfPoints = 0;
}

void bbttTextDraw(BBTT *pBBTT, int16_t _x, int16_t _y, const wchar_t _character[]) {
    uint8_t c = 0;
    uint16_t prev_code = 0;

    while (_character[c] != '\0') {
        // space (half-width, full-width)
        if ((_character[c] == ' ') || (_character[c] == L'　')) {
            prev_code = 0;
            _x += pBBTT->characterSize / 4;
            c++;
            continue;
        }

        pBBTT->charCode = bbttCodeToGlyphId(pBBTT, _character[c]);

        //Serial.printf("code:%4d\n", charCode);
        bbttReadGlyph(pBBTT, pBBTT->charCode, 0);

        _x += pBBTT->characterSpace;
#ifdef ENABLEKERNING
        if (prev_code != 0 && kerningOn) {
            int16_t kern = getKerning(prev_code, charCode);  // space between charctor
            _x += (kern * (int16_t)characterSize) / headTable.unitsPerEm;
        }
#endif
        prev_code = pBBTT->charCode;

        ttHMetric_t hMetric = bbttGetHMetric(pBBTT, pBBTT->charCode);

        // Line breaks when reaching the edge of the display
        if (c > 0 && (hMetric.advanceWidth + _x) > pBBTT->end_x) {
            _x = pBBTT->start_x;
            _y += pBBTT->characterSize;
            if (_y > pBBTT->end_y) {
                break;
            }
        }

        // Line breaks with line feed code
        if (_character[c] == '\n') {
            _x = pBBTT->start_x;
            _y += pBBTT->characterSize;
            if (_y > pBBTT->end_y) {
                break;
            }
            continue;
        }

        if (pBBTT->glyph.numberOfContours >= 0) {
            bbttGenerateOutline(pBBTT, _x, _y, pBBTT->characterSize);
            bbttFillGlyph(pBBTT, _x, _y, pBBTT->characterSize);
            if (pBBTT->colorLine != pBBTT->colorInside) {
                bbttDrawOutline(pBBTT, _x, _y, pBBTT->characterSize);
            }
        }
        pBBTT->numPoints = pBBTT->numBeginPoints = pBBTT->numEndPoints = 0; // reset for next pass
        bbttFreeGlyph(pBBTT);

        _x += hMetric.advanceWidth;
        c++;
    }
} /* bbttTextDraw() */

/* read table directory */
int bbttReadTableDirectory(BBTT *pBBTT, int checkCheckSum) {
    bbttSeek(pBBTT, numTablesPos);
    pBBTT->numTables = bbttGetUInt16t(pBBTT);
    pBBTT->table = (ttTable_t *)malloc(sizeof(ttTable_t) * pBBTT->numTables);
    bbttSeek(pBBTT, tablePos);
    for (int i = 0; i < pBBTT->numTables; i++) {
        for (int j = 0; j < 4; j++) {
            pBBTT->table[i].name[j] = bbttGetUInt8t(pBBTT);
        }
        pBBTT->table[i].name[4] = '\0';
        pBBTT->table[i].checkSum = bbttGetUInt32t(pBBTT);
        pBBTT->table[i].offset = bbttGetUInt32t(pBBTT);
        pBBTT->table[i].length = bbttGetUInt32t(pBBTT);
    }

    if (checkCheckSum) {
        for (int i = 0; i < pBBTT->numTables; i++) {
            if (strcmp(pBBTT->table[i].name, "head") != 0) { /* checksum of "head" is invalid */
                uint32_t c = bbttCalculateCheckSum(pBBTT, pBBTT->table[i].offset, pBBTT->table[i].length);
                if (pBBTT->table[i].checkSum != c) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

/* read head table */
void bbttReadHeadTable(BBTT *pBBTT) {
    for (int i = 0; i < pBBTT->numTables; i++) {
        if (strcmp(pBBTT->table[i].name, "head") == 0) {
            bbttSeek(pBBTT, pBBTT->table[i].offset);

            pBBTT->headTable.version = bbttGetUInt32t(pBBTT);
            pBBTT->headTable.revision = bbttGetUInt32t(pBBTT);
            pBBTT->headTable.checkSumAdjustment = bbttGetUInt32t(pBBTT);
            pBBTT->headTable.magicNumber = bbttGetUInt32t(pBBTT);
            pBBTT->headTable.flags = bbttGetUInt16t(pBBTT);
            pBBTT->headTable.unitsPerEm = bbttGetUInt16t(pBBTT);
            for (int j = 0; j < 8; j++) {
                pBBTT->headTable.created[i] = bbttGetUInt8t(pBBTT);
            }
            for (int j = 0; j < 8; j++) {
                pBBTT->headTable.modified[i] = bbttGetUInt8t(pBBTT);
            }
            pBBTT->xMin = pBBTT->headTable.xMin = bbttGetInt16t(pBBTT);
            pBBTT->yMin = pBBTT->headTable.yMin = bbttGetInt16t(pBBTT);
            pBBTT->xMax = pBBTT->headTable.xMax = bbttGetInt16t(pBBTT);
            pBBTT->yMax = pBBTT->headTable.yMax = bbttGetInt16t(pBBTT);
            pBBTT->headTable.macStyle = bbttGetUInt16t(pBBTT);
            pBBTT->headTable.lowestRecPPEM = bbttGetUInt16t(pBBTT);
            pBBTT->headTable.fontDirectionHint = bbttGetInt16t(pBBTT);
            pBBTT->headTable.indexToLocFormat = bbttGetInt16t(pBBTT);
            pBBTT->headTable.glyphDataFormat = bbttGetInt16t(pBBTT);
        }
    }
}
/* read cmap format 4 */
uint8_t bbttReadCmapFormat4(BBTT *pBBTT) {
    bbttSeek(pBBTT, pBBTT->cmapFormat4.offset);
    if ((pBBTT->cmapFormat4.format = bbttGetUInt16t(pBBTT)) != 4) {
        return 0;
    }

    pBBTT->cmapFormat4.length = bbttGetUInt16t(pBBTT);
    pBBTT->cmapFormat4.language = bbttGetUInt16t(pBBTT);
    pBBTT->cmapFormat4.segCountX2 = bbttGetUInt16t(pBBTT);
    pBBTT->cmapFormat4.searchRange = bbttGetUInt16t(pBBTT);
    pBBTT->cmapFormat4.entrySelector = bbttGetUInt16t(pBBTT);
    pBBTT->cmapFormat4.rangeShift = bbttGetUInt16t(pBBTT);
    pBBTT->cmapFormat4.endCodeOffset = pBBTT->cmapFormat4.offset + 14;
    pBBTT->cmapFormat4.startCodeOffset = pBBTT->cmapFormat4.endCodeOffset + pBBTT->cmapFormat4.segCountX2 + 2;
    pBBTT->cmapFormat4.idDeltaOffset = pBBTT->cmapFormat4.startCodeOffset + pBBTT->cmapFormat4.segCountX2;
    pBBTT->cmapFormat4.idRangeOffsetOffset = pBBTT->cmapFormat4.idDeltaOffset + pBBTT->cmapFormat4.segCountX2;
    pBBTT->cmapFormat4.glyphIndexArrayOffset = pBBTT->cmapFormat4.idRangeOffsetOffset + pBBTT->cmapFormat4.segCountX2;

    return 1;
}
/* read cmap */
uint8_t bbttReadCmap(BBTT *pBBTT) {
    uint16_t platformId, platformSpecificId;
    uint32_t cmapOffset, tableOffset;
    uint8_t foundMap = 0;

    if ((cmapOffset = bbttSeekToTable(pBBTT, "cmap")) == 0) {
        return 0;
    }

    pBBTT->cmapIndex.version = bbttGetUInt16t(pBBTT);
    pBBTT->cmapIndex.numberSubtables = bbttGetUInt16t(pBBTT);

    for (uint16_t i = 0; i < pBBTT->cmapIndex.numberSubtables; i++) {
        platformId = bbttGetUInt16t(pBBTT);
        platformSpecificId = bbttGetUInt16t(pBBTT);
        tableOffset = bbttGetUInt32t(pBBTT);
        if ((platformId == 3) && (platformSpecificId == 1)) {
            pBBTT->cmapFormat4.offset = cmapOffset + tableOffset;
            bbttReadCmapFormat4(pBBTT);
            foundMap = 1;
            break;
        }
    }

    if (foundMap == 0) {
        return 0;
    }

    return 1;
} /* bbttReadCmap() */

uint8_t bbttSetTtfPointer(BBTT *pBBTT, uint8_t *p, uint32_t u32Size, uint8_t _checkCheckSum) {
    pBBTT->pTTF = p;
    pBBTT->u32TTFSize = u32Size;

    if (bbttReadTableDirectory(pBBTT, _checkCheckSum) == 0) {
#ifdef ESP32
        file.close();
#endif
        return 0;
    }

    if (bbttReadCmap(pBBTT) == 0) {
#ifdef ESP32
        file.close();
#endif
        return 0;
    }

    if (bbttReadHMetric(pBBTT) == 0) {
#ifdef ESP32
        file.close();
#endif
        return 0;
    }

#ifdef ENABLEKERNING
    readKern();
#endif
    bbttReadHeadTable(pBBTT);
    return 1;
}

void bbttEnd(BBTT *pBBTT) {
#ifdef ESP32
    file.close();
#endif
    bbttFreeGlyph(pBBTT);
    if (pBBTT->table != nullptr) free(pBBTT->table);
}

void bbttSetFramebuffer(BBTT *pBBTT, uint16_t _framebufferWidth, uint16_t _framebufferHeight, uint16_t _framebuffer_bit, uint8_t *_framebuffer) {
    pBBTT->displayWidth = _framebufferWidth;
    pBBTT->displayHeight = _framebufferHeight;
    pBBTT->framebufferBit = _framebuffer_bit;
    pBBTT->userFrameBuffer = _framebuffer;

    switch (pBBTT->framebufferBit) {
        case 16:  // 16bit horizontal
            pBBTT->displayWidthFrame = pBBTT->displayWidth * 2;
            break;
        case 8:  // 8bit Horizontal
            pBBTT->displayWidthFrame = pBBTT->displayWidth;
            break;
        case 4:  // 4bit Horizontal
            pBBTT->displayWidthFrame = (pBBTT->displayWidth + 1) / 2;
            break;
        case 1:  // 1bit Horizontal
        default:
            pBBTT->displayWidthFrame = (pBBTT->displayWidth + 7) / 8;
            break;
    }
} /* bbttSetFramebuffer() */

#ifdef ESP32
uint8_t bbttSetTtfFile(BBTT *pBBTT, File _file, uint8_t _checkCheckSum) {
    if (_file == 0) {
        return 0;
    }
    pBBTT->file = _file;
    if (bbttReadTableDirectory(pBBTT, _checkCheckSum) == 0) {
        _file.close();
        return 0;
    }

    if (bbttReadCmap(pBBTT) == 0) {
        _file.close();
        return 0;
    }

    if (bbttReadHMetric(pBBTT) == 0) {
        _file.close();
        return 0;
    }
    pBBTT->iBufferedBytes = 0;
#ifdef ENABLEKERNING
    bbttReadKern(pBBTT);
#endif
    bbttReadHeadTable(pBBTT);
    bbttReadHhea(pBBTT);
    return 1;
} /* bbttSetTtfFile() */
#endif // ESP32

void bbttSetCharacterSize(BBTT *pBBTT, uint16_t _characterSize) {
    pBBTT->characterSize = _characterSize * 2; // this needs to be doubled to match the freetype point size
}

void bbttSetCharacterSpacing(BBTT *pBBTT, int16_t _characterSpace, uint8_t _kerning) {
    pBBTT->characterSpace = _characterSpace;
    pBBTT->kerningOn = _kerning;
}

#ifdef ENABLEKERNING
/* read kerning table */
uint8_t bbttReadKern(BBTT *pBBTT) {
    uint32_t nextTable;

    if (bbttSeekToTable(pBBTT, "kern") == 0) {
        return 0;
    }
    pBBTT->kernHeader.nTables = bbttGetUInt32t(pBBTT);

    // only support up to 32 sub-tables
    if (pBBTT->kernHeader.nTables > 32) {
        pBBTT->kernHeader.nTables = 32;
    }

    for (uint8_t i = 0; i < pBBTT->kernHeader.nTables; i++) {
        uint16_t format;

        pBBTT->kernSubtable.length = bbttGetUInt32t(pBBTT);
        nextTable = bbttPosition(pBBTT) + pBBTT->kernSubtable.length;
        pBBTT->kernSubtable.coverage = bbttGetUInt16t(pBBTT);

        pBBTT->format = (uint16_t)(pBBTT->kernSubtable.coverage >> 8);

        // only support format0
        if (pBBTT->format != 0) {
            bbttSeek(pBBTT, nextTable);
            continue;
        }

        // only use horizontal kerning tables
        if ((pBBTT->kernSubtable.coverage & 0x0003) != 0x0001) {
            bbttSeek(pBBTT, nextTable);
            continue;
        }

        // format0
        pBBTT->kernFormat0.nPairs = bbttGetUInt16t(pBBTT);
        pBBTT->kernFormat0.searchRange = bbttGetUInt16t(pBBTT);
        pBBTT->kernFormat0.entrySelector = bbttGetUInt16t(pBBTT);
        pBBTT->kernFormat0.rangeShift = bbttGetUInt16t(pBBTT);
        pBBTT->kernTablePos = bbttPosition(pBBTT);

        break;
    }
    return 1;
} /* bbttReadKern() */

int16_t bbttGetKerning(BBTT *pBBTT, uint16_t _left_glyph, uint16_t _right_glyph) {
    if (pBBTT->kernTablePos == 0) return 0;
    int16_t result = 0;
    uint32_t key0 = ((uint32_t)(_left_glyph) << 16) | (_right_glyph);
    bbttSeek(pBBTT, pBBTT->kernTablePos);
    for (uint16_t i = 0; i < pBBTT->kernFormat0.nPairs; i++) {
        uint32_t key1 = bbttGetUInt32t(pBBTT);
        if (key0 == key1) {
            result = bbttGetInt16t(pBBTT);
            break;
        }
        uint16_t dummy = bbttGetInt16t(pBBTT);
    }
    return result;
} /* bbttGetKerning() */
#endif
