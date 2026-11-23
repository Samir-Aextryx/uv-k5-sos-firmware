/* Copyright 2023 Dual Tachyon
 * https://github.com/DualTachyon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#include <string.h>

#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "font.h"
#include "ui/helper.h"
#include "ui/inputbox.h"
#include "misc.h"

#ifndef ARRAY_SIZE
    #define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))
#endif

void UI_GenerateChannelString(char *pString, const uint8_t Channel)
{
    unsigned int i;

    if (gInputBoxIndex == 0)
    {
        sprintf(pString, "CH-%02u", Channel + 1);
        return;
    }

    pString[0] = 'C';
    pString[1] = 'H';
    pString[2] = '-';
    for (i = 0; i < 2; i++)
        pString[i + 3] = (gInputBox[i] == 10) ? '-' : gInputBox[i] + '0';
}

void UI_GenerateChannelStringEx(char *pString, const bool bShowPrefix, const uint8_t ChannelNumber)
{
    if (gInputBoxIndex > 0) {
        for (unsigned int i = 0; i < 3; i++) {
            pString[i] = (gInputBox[i] == 10) ? '-' : gInputBox[i] + '0';
        }

        pString[3] = 0;
        return;
    }

    if (bShowPrefix) {
        // BUG here? Prefixed NULLs are allowed
        sprintf(pString, "CH-%03u", ChannelNumber + 1);
    } else if (ChannelNumber == 0xFF) {
        strcpy(pString, "NULL");
    } else {
        sprintf(pString, "%03u", ChannelNumber + 1);
    }
}

// Umlaute kommen als UTF-8 (0xC3 + Folgebyte). Nummer 0..6 = ae oe ue AE OE UE ss, sonst -1
int UI_Umlaut(uint8_t next)
{
    static const uint8_t utf8[7] = {0xA4, 0xB6, 0xBC, 0x84, 0x96, 0x9C, 0x9F};
    for (int k = 0; k < 7; k++)
        if (utf8[k] == next)
            return k;
    return -1;
}

// Laenge in Zeichen (ein Umlaut = 2 Bytes, aber 1 Zeichen)
size_t UI_StrLen(const char *pString)
{
    size_t n = 0;
    for (const uint8_t *p = (const uint8_t *)pString; *p; p++)
        if ((*p & 0xC0) != 0x80)
            n++;
    return n;
}

void UI_PrintStringBuffer(const char *pString, uint8_t * buffer, uint32_t char_width, const uint8_t *font)
{
    const unsigned int char_spacing = char_width + 1;
#ifdef ENABLE_SMALL_BOLD
    const uint8_t *uml = (font == (const uint8_t *)gFontSmallBold) ? &gFontSmallBoldUml[0][0] : &gFontSmallUml[0][0];
#else
    const uint8_t *uml = &gFontSmallUml[0][0];
#endif
    unsigned int i = 0;
    for (const uint8_t *p = (const uint8_t *)pString; *p; p++) {
        const uint8_t c = *p;
        const uint8_t *glyph = NULL;
        if ((c & 0xC0) == 0x80)
            continue;                          // Folgebyte eines Umlauts
        if (c > ' ' && c < 127)
            glyph = font + (c - ' ' - 1) * char_width;
        else if (c == 0xC3 && char_width == 6) {
            const int k = UI_Umlaut(p[1]);
            if (k >= 0)
                glyph = uml + k * 6;
        }
        if (glyph)
            memcpy(buffer + i * char_spacing + 1, glyph, char_width);
        i++;
    }
}

void UI_PrintString(const char *pString, uint8_t Start, uint8_t End, uint8_t Line, uint8_t Width)
{
    size_t i;
    size_t Length = strlen(pString);

    if (End > Start)
        Start += (((End - Start) - (Length * Width)) + 1) / 2;

    for (i = 0; i < Length; i++)
    {
        const unsigned int ofs   = (unsigned int)Start + (i * Width);
        if (pString[i] > ' ' && pString[i] < 127)
        {
            const unsigned int index = pString[i] - ' ' - 1;
            memcpy(gFrameBuffer[Line + 0] + ofs, &gFontBig[index][0], 7);
            memcpy(gFrameBuffer[Line + 1] + ofs, &gFontBig[index][7], 7);
        }
    }
}

void UI_PrintStringSmall(const char *pString, uint8_t Start, uint8_t End, uint8_t Line, uint8_t char_width, const uint8_t *font)
{
    const size_t Length = UI_StrLen(pString);
    const unsigned int char_spacing = char_width + 1;

    if (End > Start) {
        Start += (((End - Start) - Length * char_spacing) + 1) / 2;
    }

    UI_PrintStringBuffer(pString, gFrameBuffer[Line] + Start, char_width, font);
}


void UI_PrintStringSmallNormal(const char *pString, uint8_t Start, uint8_t End, uint8_t Line)
{
    UI_PrintStringSmall(pString, Start, End, Line, ARRAY_SIZE(gFontSmall[0]), (const uint8_t *)gFontSmall);
}

void UI_PrintStringSmallBold(const char *pString, uint8_t Start, uint8_t End, uint8_t Line)
{
#ifdef ENABLE_SMALL_BOLD
    const uint8_t *font = (uint8_t *)gFontSmallBold;
    const uint8_t char_width = ARRAY_SIZE(gFontSmallBold[0]);
#else
    const uint8_t *font = (uint8_t *)gFontSmall;
    const uint8_t char_width = ARRAY_SIZE(gFontSmall[0]);
#endif

    UI_PrintStringSmall(pString, Start, End, Line, char_width, font);
}

void UI_PrintStringSmallBufferNormal(const char *pString, uint8_t * buffer)
{
    UI_PrintStringBuffer(pString, buffer, ARRAY_SIZE(gFontSmall[0]), (uint8_t *)gFontSmall);
}

void UI_PrintStringSmallBufferBold(const char *pString, uint8_t * buffer)
{
#ifdef ENABLE_SMALL_BOLD
    const uint8_t *font = (uint8_t *)gFontSmallBold;
    const uint8_t char_width = ARRAY_SIZE(gFontSmallBold[0]);
#else
    const uint8_t *font = (uint8_t *)gFontSmall;
    const uint8_t char_width = ARRAY_SIZE(gFontSmall[0]);
#endif
    UI_PrintStringBuffer(pString, buffer, char_width, font);
}

void UI_DisplayFrequency(const char *string, uint8_t X, uint8_t Y, bool center)
{
    const unsigned int char_width  = 13;
    uint8_t           *pFb0        = gFrameBuffer[Y] + X;
    uint8_t           *pFb1        = pFb0 + 128;
    bool               bCanDisplay = false;

    uint8_t len = strlen(string);
    for(int i = 0; i < len; i++) {
        char c = string[i];
        if(c=='-') c = '9' + 1;
        if (bCanDisplay || c != ' ')
        {
            bCanDisplay = true;
            if(c>='0' && c<='9' + 1) {
                memcpy(pFb0 + 2, gFontBigDigits[c-'0'],                  char_width - 3);
                memcpy(pFb1 + 2, gFontBigDigits[c-'0'] + char_width - 3, char_width - 3);
            }
            else if(c=='.') {
                *pFb1 = 0x60; pFb0++; pFb1++;
                *pFb1 = 0x60; pFb0++; pFb1++;
                *pFb1 = 0x60; pFb0++; pFb1++;
                continue;
            }

        }
        else if (center) {
            pFb0 -= 6;
            pFb1 -= 6;
        }
        pFb0 += char_width;
        pFb1 += char_width;
    }
}

/*
void UI_DisplayFrequency(const char *string, uint8_t X, uint8_t Y, bool center)
{
    const unsigned int char_width  = 13;
    uint8_t           *pFb0        = gFrameBuffer[Y] + X;
    uint8_t           *pFb1        = pFb0 + 128;
    bool               bCanDisplay = false;

    if (center) {
        uint8_t len = 0;
        for (const char *ptr = string; *ptr; ptr++)
            if (*ptr != ' ') len++; // Ignores spaces for centering

        X -= (len * char_width) / 2; // Centering adjustment
        pFb0 = gFrameBuffer[Y] + X;
        pFb1 = pFb0 + 128;
    }

    for (; *string; string++) {
        char c = *string;
        if (c == '-') c = '9' + 1; // Remap of '-' symbol

        if (bCanDisplay || c != ' ') {
            bCanDisplay = true;
            if (c >= '0' && c <= '9' + 1) {
                memcpy(pFb0 + 2, gFontBigDigits[c - '0'], char_width - 3);
                memcpy(pFb1 + 2, gFontBigDigits[c - '0'] + char_width - 3, char_width - 3);
            } else if (c == '.') {
                memset(pFb1, 0x60, 3); // Replaces the three assignments
                pFb0 += 3;
                pFb1 += 3;
                continue;
            }
        }
        pFb0 += char_width;
        pFb1 += char_width;
    }
}
*/

void UI_DrawPixelBuffer(uint8_t (*buffer)[128], uint8_t x, uint8_t y, bool black)
{
    const uint8_t pattern = 1 << (y % 8);
    if(black)
        buffer[y/8][x] |= pattern;
    else
        buffer[y/8][x] &= ~pattern;
}

static void sort(int16_t *a, int16_t *b)
{
    if(*a > *b) {
        int16_t t = *a;
        *a = *b;
        *b = t;
    }
}

#ifdef ENABLE_FEAT_F4HWN
    /*
    void UI_DrawLineDottedBuffer(uint8_t (*buffer)[128], int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool black)
    {
        if(x2==x1) {
            sort(&y1, &y2);
            for(int16_t i = y1; i <= y2; i+=2) {
                UI_DrawPixelBuffer(buffer, x1, i, black);
            }
        } else {
            const int multipl = 1000;
            int a = (y2-y1)*multipl / (x2-x1);
            int b = y1 - a * x1 / multipl;

            sort(&x1, &x2);
            for(int i = x1; i<= x2; i+=2)
            {
                UI_DrawPixelBuffer(buffer, i, i*a/multipl +b, black);
            }
        }
    }
    */

    void PutPixel(uint8_t x, uint8_t y, bool fill) {
      UI_DrawPixelBuffer(gFrameBuffer, x, y, fill);
    }

    void PutPixelStatus(uint8_t x, uint8_t y, bool fill) {
      UI_DrawPixelBuffer(&gStatusLine, x, y, fill);
    }

    void GUI_DisplaySmallest(const char *pString, uint8_t x, uint8_t y,
                                    bool statusbar, bool fill) {
      uint8_t c;
      uint8_t pixels;
      const uint8_t *p = (const uint8_t *)pString;

      while ((c = *p++) && c != '\0') {
        const uint8_t *glyph;
        if ((c & 0xC0) == 0x80)
          continue;                            // Folgebyte eines Umlauts
        if (c == 0xC3 && UI_Umlaut(*p) >= 0)
          glyph = gFont3x5Uml[UI_Umlaut(*p)];
        else if (c >= 0x20 && c < 0x80)
          glyph = gFont3x5[c - 0x20];
        else
          glyph = gFont3x5[0];
        for (int i = 0; i < 3; ++i) {
          pixels = glyph[i];
          for (int j = 0; j < 6; ++j) {
            if (pixels & 1) {
              if (statusbar)
                PutPixelStatus(x + i, y + j, fill);
              else
                PutPixel(x + i, y + j, fill);
            }
            pixels >>= 1;
          }
        }
        x += 4;
      }
    }
#endif
    
// Zeile von x1 bis x2 invertieren und die Ecken abrunden (Auswahl-Balken)
void UI_InvertBar(uint8_t line, uint8_t x1, uint8_t x2)
{
    for (uint8_t x = x1; x <= x2; x++)
        gFrameBuffer[line][x] ^= 0xFF;
    gFrameBuffer[line][x1] &= 0x7E;
    gFrameBuffer[line][x2] &= 0x7E;
}

#if defined(ENABLE_QUICKMENU) || defined(ENABLE_SAMIR_LOGO)
// Die vier Eckpunkte eines Rechtecks loeschen (runde Ecken)
static void ClearCorners(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    UI_DrawPixelBuffer(gFrameBuffer, x1, y1, false);
    UI_DrawPixelBuffer(gFrameBuffer, x2, y1, false);
    UI_DrawPixelBuffer(gFrameBuffer, x1, y2, false);
    UI_DrawPixelBuffer(gFrameBuffer, x2, y2, false);
}

// Rechteck invertieren, mit runden Ecken (gewaehlte Kachel, Leuchtfeld)
void UI_InvertRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    for (uint8_t x = x1; x <= x2; x++)
        for (uint8_t y = y1; y <= y2; y++)
            gFrameBuffer[y >> 3][x] ^= 1u << (y & 7);
    ClearCorners(x1, y1, x2, y2);
}

// Rahmen mit runden Ecken
void UI_DrawRoundRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    UI_DrawRectangleBuffer(gFrameBuffer, x1, y1, x2, y2, true);
    ClearCorners(x1, y1, x2, y2);
}

// eine Spalte der Bitmap als Zahl (Bit 0 = oben), ausserhalb = leer
static uint16_t BitmapColumn(const uint8_t *bmp, uint8_t w, uint8_t h, int8_t c)
{
    if (c < 0 || c >= w)
        return 0;
    return bmp[c] | (h > 8 ? bmp[c + w] << 8 : 0);
}

// Bitmap (spaltenweise, Bit 0 = oben, je 8 Zeilen ein Block von w Bytes)
// doppelt so gross zeichnen. Schraegen werden dabei geglaettet (Scale2x),
// damit grosse Buchstaben und Symbole nicht treppig aussehen.
void UI_DrawScale2x(const uint8_t *bmp, uint8_t w, uint8_t h, uint8_t x, uint8_t y, bool black)
{
    for (int8_t c = 0; c < w; c++) {
        // um 1 verschoben: Bit r+1 = Zeile r, damit Zeile -1 und h leer sind
        const uint32_t L = BitmapColumn(bmp, w, h, c - 1) << 1;
        const uint32_t M = BitmapColumn(bmp, w, h, c) << 1;
        const uint32_t R = BitmapColumn(bmp, w, h, c + 1) << 1;
        for (uint8_t r = 0; r < h; r++) {
            const bool P = (M >> (r + 1)) & 1, A = (M >> r) & 1, D = (M >> (r + 2)) & 1;
            const bool B = (R >> (r + 1)) & 1, C = (L >> (r + 1)) & 1;
            const bool e[4] = {
                (C == A && C != D && A != B) ? A : P,
                (A == B && A != C && B != D) ? B : P,
                (D == C && D != B && C != A) ? C : P,
                (B == D && B != A && D != C) ? D : P,
            };
            for (uint8_t d = 0; d < 4; d++) {
                const uint8_t px = x + 2 * c + (d & 1), py = y + 2 * r + (d >> 1);
                if (e[d] && px < 128 && py < 56)   // ausserhalb: weglassen
                    UI_DrawPixelBuffer(gFrameBuffer, px, py, black);
            }
        }
    }
}
#endif

void UI_DrawLineBuffer(uint8_t (*buffer)[128], int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool black)
{
    if(x2==x1) {
        sort(&y1, &y2);
        for(int16_t i = y1; i <= y2; i++) {
            UI_DrawPixelBuffer(buffer, x1, i, black);
        }
    } else {
        const int multipl = 1000;
        int a = (y2-y1)*multipl / (x2-x1);
        int b = y1 - a * x1 / multipl;

        sort(&x1, &x2);
        for(int i = x1; i<= x2; i++)
        {
            UI_DrawPixelBuffer(buffer, i, i*a/multipl +b, black);
        }
    }
}

void UI_DrawRectangleBuffer(uint8_t (*buffer)[128], int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool black)
{
    UI_DrawLineBuffer(buffer, x1,y1, x1,y2, black);
    UI_DrawLineBuffer(buffer, x1,y1, x2,y1, black);
    UI_DrawLineBuffer(buffer, x2,y1, x2,y2, black);
    UI_DrawLineBuffer(buffer, x1,y2, x2,y2, black);
}


void UI_DisplayPopup(const char *string)
{
    UI_DisplayClear();

    // for(uint8_t i = 1; i < 5; i++) {
    //  memset(gFrameBuffer[i]+8, 0x00, 111);
    // }

    // for(uint8_t x = 10; x < 118; x++) {
    //  UI_DrawPixelBuffer(x, 10, true);
    //  UI_DrawPixelBuffer(x, 46-9, true);
    // }

    // for(uint8_t y = 11; y < 37; y++) {
    //  UI_DrawPixelBuffer(10, y, true);
    //  UI_DrawPixelBuffer(117, y, true);
    // }
    // DrawRectangle(9,9, 118,38, true);
    UI_PrintString(string, 9, 118, 2, 8);
    UI_PrintStringSmallNormal("Press EXIT", 9, 118, 6);
}

void UI_DisplayClear()
{
    memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
}
