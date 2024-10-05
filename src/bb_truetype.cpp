#include "truetype.h"
#include "bbtt.inl"

//Kerning is optional. Many fonts don't have kerning tables anyway.
//#define ENABLEKERNING

bb_truetype::bb_truetype() {
    bbttInit(&_bbtt);
}

void bb_truetype::end() {
    bbttEnd(&_bbtt);
}

#ifdef ESP32
uint8_t bb_truetype::setTtfFile(File _file, uint8_t _checkCheckSum)
{
    return bbttSetTtfFile(&_bbtt, _file, _checkCheckSum);
}
#endif // ESP32

void bb_truetype::setTtfDrawLine(TTF_DRAWLINE *p) {
    _bbtt.pfnDrawLine = p;
}

uint8_t bb_truetype::setTtfPointer(uint8_t *p, uint32_t u32Size, uint8_t _checkCheckSum){
    return bbttSetTtfPointer(&_bbtt, p, u32Size, _checkCheckSum);
}

void bb_truetype::setFramebuffer(uint16_t _framebufferWidth, uint16_t _framebufferHeight, uint16_t _framebuffer_bit, uint8_t *_framebuffer)
{
    bbttSetFramebuffer(&_bbtt, _framebufferWidth, _framebufferHeight, _framebuffer_bit, _framebuffer);
}

void bb_truetype::setCharacterSize(uint16_t _characterSize) {
    bbttSetCharacterSize(&_bbtt, _characterSize);
}

void bb_truetype::setCharacterSpacing(int16_t _characterSpace, uint8_t _kerning) {
    bbttSetCharacterSpacing(&_bbtt, _characterSpace, _kerning);
}

void bb_truetype::setTextBoundary(uint16_t _start_x, uint16_t _end_x, uint16_t _end_y) {
    bbttSetTextBoundary(&_bbtt, _start_x, _end_x, _end_y);
}

void bb_truetype::setTextColor(uint16_t _onLine, uint16_t _inside) {
    bbttSetTextColor(&_bbtt, _onLine, _inside);
}

void bb_truetype::setTextRotation(uint16_t _rotation) {
    bbttSetRotation(&_bbtt, _rotation);
}

void bb_truetype::textDraw(int16_t _x, int16_t _y, const wchar_t _character[]) {
    bbttTextDraw(&_bbtt, _x, _y, _character);
}

void bb_truetype::textDraw(int16_t _x, int16_t _y, const char _character[]) {
    uint16_t length = 0;
    while (_character[length] != '\0') {
        length++;
    }
    wchar_t *wcharacter = (wchar_t *)calloc(sizeof(wchar_t), length + 1);
    for (uint16_t i = 0; i < length; i++) {
        wcharacter[i] = _character[i];
    }
    bbttTextDraw(&_bbtt, _x, _y, wcharacter);
    free(wcharacter);
    wcharacter = nullptr;
}

uint16_t bb_truetype::getStringWidth(const wchar_t *szwString) {
    return bbttGetStringWidthW(&_bbtt, szwString);
}

uint16_t bb_truetype::getStringWidth(const char *szString) {
    return bbttGetStringWidth(&_bbtt, szString);
}

#ifdef ARDUINO
void bb_truetype::textDraw(int16_t _x, int16_t _y, const String _string) {
    uint16_t length = _string.length();
    wchar_t *wcharacter = (wchar_t *)calloc(sizeof(wchar_t), length + 1);
    stringToWchar(_string, wcharacter);
    bbttTextDraw(&_bbtt, _x, _y, wcharacter);
    free(wcharacter);
    wcharacter = nullptr;
}

uint16_t bb_truetype::getStringWidth(const String _string) {
    uint16_t length = _string.length();
    uint16_t output = 0;

    wchar_t *wcharacter = (wchar_t *)calloc(sizeof(wchar_t), length + 1);
    stringToWchar(_string, wcharacter);

    output = bbttGetStringWidth(&_bbtt, wcharacter);
    free(wcharacter);
    wcharacter = nullptr;
    return output;
}

/* calculate */
void bb_truetype::stringToWchar(String _string, wchar_t _charctor[]) {
    uint16_t s = 0;
    uint8_t c = 0;
    uint32_t codeu32;

    while (_string[s] != '\0') {
        int numBytes = GetU8ByteCount(_string[s]);
        switch (numBytes) {
            case 1:
                codeu32 = char32_t(uint8_t(_string[s]));
                s++;
                break;
            case 2:
                if (!IsU8LaterByte(_string[s + 1])) {
                    continue;
                }
                if ((uint8_t(_string[s]) & 0x1E) == 0) {
                    continue;
                }

                codeu32 = char32_t(_string[s] & 0x1F) << 6;
                codeu32 |= char32_t(_string[s + 1] & 0x3F);
                s += 2;
                break;
            case 3:
                if (!IsU8LaterByte(_string[s + 1]) || !IsU8LaterByte(_string[s + 2])) {
                    continue;
                }
                if ((uint8_t(_string[s]) & 0x0F) == 0 &&
                    (uint8_t(_string[s + 1]) & 0x20) == 0) {
                    continue;
                }

                codeu32 = char32_t(_string[s] & 0x0F) << 12;
                codeu32 |= char32_t(_string[s + 1] & 0x3F) << 6;
                codeu32 |= char32_t(_string[s + 2] & 0x3F);
                s += 3;
                break;
            case 4:
                if (!IsU8LaterByte(_string[s + 1]) || !IsU8LaterByte(_string[s + 2]) ||
                    !IsU8LaterByte(_string[s + 3])) {
                    continue;
                }
                if ((uint8_t(_string[s]) & 0x07) == 0 &&
                    (uint8_t(_string[s + 1]) & 0x30) == 0) {
                    continue;
                }

                codeu32 = char32_t(_string[s] & 0x07) << 18;
                codeu32 |= char32_t(_string[s + 1] & 0x3F) << 12;
                codeu32 |= char32_t(_string[s + 2] & 0x3F) << 6;
                codeu32 |= char32_t(_string[s + 3] & 0x3F);
                s += 4;
                break;
            default:
                continue;
        }

        if (codeu32 < 0 || codeu32 > 0x10FFFF) {
            continue;
        }

        if (codeu32 < 0x10000) {
            _charctor[c] = char16_t(codeu32);
        } else {
            _charctor[c] = ((char16_t((codeu32 - 0x10000) % 0x400 + 0xDC00)) << 8) || (char16_t((codeu32 - 0x10000) / 0x400 + 0xD800));
        }
        c++;
    }
    _charctor[c] = 0;
}

uint8_t bb_truetype::GetU8ByteCount(char _ch) {
    if (0 <= uint8_t(_ch) && uint8_t(_ch) < 0x80) {
        return 1;
    }
    if (0xC2 <= uint8_t(_ch) && uint8_t(_ch) < 0xE0) {
        return 2;
    }
    if (0xE0 <= uint8_t(_ch) && uint8_t(_ch) < 0xF0) {
        return 3;
    }
    if (0xF0 <= uint8_t(_ch) && uint8_t(_ch) < 0xF8) {
        return 4;
    }
    return 0;
}

bool bb_truetype::IsU8LaterByte(char _ch) {
    return 0x80 <= uint8_t(_ch) && uint8_t(_ch) < 0xC0;
}
#endif // ARDUINO
