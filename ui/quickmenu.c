/* Schnellwahl-Menue: Anzeige
 *
 * Startseite: sechs Kacheln (3 oben, 3 unten) mit grossem Symbol, Name und
 *             Ziffer fuer die Direktwahl. Die gewaehlte Kachel ist schwarz.
 * Listen:     Titelleiste mit Symbol, Eintraege mit Wert rechts (AN/AUS).
 * Seiten:     UKW-Radio, Hoeren, Amateurfunk, Akku & Info, SOS-Ruf.
 * Unten steht immer, was die Tasten tun.
 *
 * Licensed under the Apache License, Version 2.0
 */

#ifdef ENABLE_QUICKMENU

#include <string.h>

#include "app/quickmenu.h"
#ifdef ENABLE_FMRADIO
    #include "app/fm.h"
#endif
#include "app/chFrScanner.h"
#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "functions.h"
#include "ui/helper.h"
#include "ui/quickmenu.h"
#include "ui/ui.h"
#include "helper/battery.h"
#include "misc.h"
#include "radio.h"
#include "settings.h"
#include "version.h"

#define QM_ROWS 6   // sichtbare Eintraege in Listen

// 8x8-Symbole fuer die Titelleiste, spaltenweise, Bit 0 = oben
const uint8_t gQM_Icons[][8] = {
    {0x7C, 0x44, 0x54, 0x7C, 0x44, 0x56, 0x45, 0x7C},   // Radio
    {0x1C, 0x08, 0x08, 0x7F, 0x3E, 0x1C, 0x08, 0x08},   // Flugzeug
    {0x05, 0x42, 0x30, 0x2E, 0x30, 0x42, 0x05, 0x00},   // Funkmast
    {0xE0, 0x98, 0x86, 0xD9, 0xD9, 0x86, 0x98, 0xE0},   // Warndreieck
    {0x03, 0x05, 0x06, 0x0C, 0x18, 0x70, 0xA0, 0xE0},   // Schraubenschluessel
    {0x18, 0x7E, 0x7E, 0xC3, 0xC3, 0x7E, 0x7E, 0x18},   // Zahnrad
};

// 16x16-Symbole fuer die Kacheln: erst 16 Bytes fuer die Zeilen 0..7,
// dann 16 Bytes fuer die Zeilen 8..15 (spaltenweise, Bit 0 = oben)
static const uint8_t TileIcons[][32] = {
    {0xC0, 0x20, 0xA0, 0xA0, 0xA0, 0xA0, 0x20, 0x20, 0x20, 0x30, 0xA8, 0xA4, 0xA2, 0x20, 0x20, 0xC0, 0x1F, 0x20, 0x6F, 0x6A, 0x2A, 0x2F, 0x20, 0x20, 0x20, 0x27, 0x28, 0x2A, 0x68, 0x67, 0x20, 0x1F},   // Radio
    {0x80, 0xC0, 0xC0, 0xE0, 0xE0, 0xF0, 0xFE, 0xFF, 0xFF, 0xFE, 0xF0, 0xE0, 0xE0, 0xC0, 0xC0, 0x80, 0x01, 0x01, 0x00, 0x00, 0x60, 0x70, 0x37, 0x3F, 0x3F, 0x37, 0x70, 0x60, 0x00, 0x00, 0x01, 0x01},   // Flugzeug
    {0x1C, 0x22, 0x41, 0x00, 0x1C, 0x22, 0x08, 0xFC, 0xFC, 0x08, 0x22, 0x1C, 0x00, 0x41, 0x22, 0x1C, 0x00, 0x00, 0x00, 0x40, 0x70, 0x0C, 0x0B, 0x08, 0x08, 0x0B, 0x0C, 0x70, 0x40, 0x00, 0x00, 0x00},   // Funkmast
    {0x00, 0x00, 0x00, 0x80, 0x60, 0x18, 0x06, 0xFB, 0xFB, 0x06, 0x18, 0x60, 0x80, 0x00, 0x00, 0x00, 0x60, 0x58, 0x46, 0x41, 0x40, 0x40, 0x40, 0x5B, 0x5B, 0x40, 0x40, 0x40, 0x41, 0x46, 0x58, 0x60},   // Warndreieck
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0x7C, 0x3E, 0x3F, 0x38, 0x1C, 0x0E, 0x00, 0x00, 0x30, 0x38, 0x1C, 0x0E, 0x07, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // Werkzeug
    {0xE0, 0xE0, 0xF6, 0xFE, 0xFC, 0x3C, 0x1F, 0x0F, 0x0F, 0x1F, 0x3C, 0xFC, 0xFE, 0xF6, 0xE0, 0xE0, 0x07, 0x07, 0x6F, 0x7F, 0x3F, 0x3C, 0xF8, 0xF0, 0xF0, 0xF8, 0x3C, 0x3F, 0x7F, 0x6F, 0x07, 0x07},   // Zahnrad
};

// kurze Namen fuer die Kacheln (passen in 40 Pixel)
static const char *const TileNames[] = {"RADIO", "FLUGZEUG", "AMATEUR", "SOS", "WERKZEUG", "EINSTELL."};

static void TitleBar(uint8_t icon, const char *title)
{
    memcpy(gFrameBuffer[0] + 3, gQM_Icons[icon], 8);
    UI_PrintStringSmallBold(title, 15, 0, 0);
    UI_InvertBar(0, 0, 127);
}

static void Tiny(const char *s, uint8_t y)
{
    GUI_DisplaySmallest(s, (129 - 4 * UI_StrLen(s)) / 2, y, false, true);
}

static void TinyRight(const char *s)   // rechts in der Titelleiste
{
    GUI_DisplaySmallest(s, 125 - 4 * UI_StrLen(s), 1, false, false);
}

// Frequenz gross in die Zeilen 1-2, Nullen am Ende weg (mind. 3 Stellen)
static void BigFreq(uint32_t f)
{
    char s[12];
    sprintf(s, "%u.%05u", f / 100000, f % 100000);
    char *dot = strchr(s, '.');
    for (char *e = s + strlen(s) - 1; e > dot + 3 && *e == '0'; e--)
        *e = 0;
    UI_DisplayFrequency(s, (128 - 13 * (strlen(s) - 1) - 3) / 2, 1, false);
}

// Zeile 3: Beschriftung invertiert links, daneben 10 Balken (n davon voll)
static void SignalBars(const char *label, uint8_t n)
{
    UI_PrintStringSmallBold(label, 3, 0, 3);
    UI_InvertRect(0, 24, 53, 31);
    for (uint8_t i = 0; i < 10; i++) {
        const uint8_t h    = 2 + i / 2;   // Balken werden nach rechts hoeher
        const uint8_t mask = (i < n) ? (uint8_t)(0x7F & ~((1u << (7 - h)) - 1)) : 0x40;
        for (uint8_t k = 0; k < 5; k++)
            gFrameBuffer[3][57 + i * 7 + k] |= mask;
    }
}

// Zeile 3: Was macht das Funkgeraet gerade?
static void Status(void)
{
    if (gCurrentFunction == FUNCTION_TRANSMIT) {
        UI_PrintStringSmallBold("SENDEN", 0, 127, 3);
        UI_InvertRect(24, 24, 103, 31);
    } else if (FUNCTION_IsRx()) {
        // "EMPFANG" und daneben die Signalstaerke: 10 Balken, je staerker, desto mehr
        SignalBars("EMPFANG", gQM_Signal);
    } else {
        UI_PrintStringSmallBold(gScanStateDir != SCAN_OFF ? "Suche läuft ..." : "Warte auf Signal", 0, 127, 3);
    }
}

// --- Startseite: Kacheln -----------------------------------------------------
#define TILE_W 41
#define TILE_H 27

static void DisplayTiles(void)
{
    char num[2] = {0, 0};

    for (uint8_t i = 0; i < gQM_CategoryCount; i++) {
        const uint8_t x   = 1 + (i % 3) * (TILE_W + 2);
        const uint8_t y   = (i / 3) * (TILE_H + 1);
        const bool    sel = i == gQM_Selected;

        if (sel)
            UI_InvertRect(x, y, x + TILE_W - 1, y + TILE_H - 1);
        else
            UI_DrawRoundRect(x, y, x + TILE_W - 1, y + TILE_H - 1);

        num[0] = '1' + i;
        GUI_DisplaySmallest(num, x + 3, y + 3, false, !sel);
        for (uint8_t c = 0; c < 16; c++)
            for (uint8_t r = 0; r < 16; r++)
                if ((TileIcons[i][c + (r & 8) * 2] >> (r & 7)) & 1)
                    UI_DrawPixelBuffer(gFrameBuffer, x + 12 + c, y + 3 + r, !sel);
        const char *name = TileNames[i];
        GUI_DisplaySmallest(name, x + (TILE_W + 1 - 4 * UI_StrLen(name)) / 2, y + 20, false, !sel);
    }
}

// --- Listen ------------------------------------------------------------------
static void DisplayList(void)
{
    char String[2];
    const QM_Category_t *cat = &gQM_Categories[gQM_Category];
    const uint8_t count = cat->count;

    TitleBar(gQM_Category, cat->name);

    // jede Liste hat hoechstens 6 Eintraege und passt auf den Bildschirm
    for (uint8_t i = 0; i < count && i < QM_ROWS; i++) {
        const uint8_t line = i + 1;

        String[0] = '1' + i;
        String[1] = 0;
        GUI_DisplaySmallest(String, 3, line * 8 + 1, false, true);
        UI_PrintStringSmallNormal(gQM_Items[cat->first + i].name, 10, 0, line);

        const char *v = QUICKMENU_Value(cat->first + i);
        if (v)
            UI_PrintStringSmallBold(v, 126 - 7 * UI_StrLen(v), 0, line);

        if (i == gQM_Selected)
            UI_InvertBar(line, 1, 127);
    }
}

// --- Seite "UKW-Radio" -------------------------------------------------------
#ifdef ENABLE_FMRADIO
static void DisplayRadio(void)
{
    char String[8];
    const uint16_t f = gEeprom.FM_FrequencyPlaying;   // in 100 kHz

    TitleBar(0, "UKW-RADIO");
    sprintf(String, "%u.%u", f / 10, f % 10);
    UI_DisplayFrequency(String, (128 - 13 * (strlen(String) - 1) - 3) / 2, 1, false);
    UI_PrintStringSmallBold(gFM_ScanState != FM_SCAN_OFF ? "Suche Sender ..." : "Radio läuft", 0, 127, 3);
    Tiny("HOCH/RUNTER = ANDERER SENDER", 36);
    Tiny("LAUTER/LEISER = DREHKNOPF", 43);
    Tiny("EXIT = RADIO AUS", 50);
}
#endif

// --- Seite "Hoeren" (Flugfunk, PMR, Antwort auf SOS) -------------------------
static void DisplayListen(void)
{
    char String[32];

    TitleBar(gQM_Category, gQM_Title);
    TinyRight(gRxVfo->Modulation == MODULATION_AM ? "AM" : "FM");
    BigFreq(gRxVfo->freq_config_RX.Frequency);
    Status();
    sprintf(String, "HOCH/RUNTER = RAUSCHSPERRE %u", gEeprom.SQUELCH_LEVEL);
    Tiny(String, 35);
    const char *talk = QUICKMENU_CanTalk() ? "PTT=SPRECHEN" : "NUR HÖREN";
    if (QUICKMENU_HasScan()) {
        sprintf(String, "M=SUCHE AN/AUS  %s", talk);
        Tiny(String, 42);
    } else {
        Tiny(QUICKMENU_CanTalk() ? "PTT = SPRECHEN" : "NUR HÖREN - SENDEN GESPERRT", 42);
    }
    Tiny("EXIT = ZURÜCK", 50);
}

// --- Seite "Amateurfunk" -----------------------------------------------------
static void DisplayHam(void)
{
    char String[32];
    const uint32_t f = gTxVfo->freq_config_RX.Frequency;
    const uint8_t  p = gTxVfo->OUTPUT_POWER;

    TitleBar(2, "AMATEURFUNK");
    TinyRight(QUICKMENU_HamBand(f) ? "70CM" : "2M");

    if (gQM_Input[0]) {   // Eingabe: "145.5--"
        const char *in = gQM_Input;
        for (uint8_t i = 0, o = 0; o < 7; o++) {
            if (o == 3)
                String[o] = '.';
            else
                String[o] = in[i] ? in[i++] : '-';
        }
        String[7] = 0;
        UI_DisplayFrequency(String, 23, 1, false);
        UI_PrintStringSmallBold("Frequenz tippen", 0, 127, 3);
        Tiny("6 ZIFFERN, Z.B. 145500", 36);
        Tiny("EXIT = LÖSCHEN", 50);
        return;
    }

    BigFreq(f);
    Status();
    Tiny("HOCH/RUNTER=FREQ  0-9=TIPPEN", 36);
    sprintf(String, "M=SUCHE  *=%s", p >= OUTPUT_POWER_HIGH ? "LEISTUNG HOCH" : p >= OUTPUT_POWER_MID ? "LEISTUNG MITTEL" : "LEISTUNG NIEDRIG");
    Tiny(String, 43);
    Tiny("PTT = SPRECHEN   EXIT = ZURÜCK", 50);
}

// --- Seite "Akku & Info" -----------------------------------------------------
static void DisplayInfo(void)
{
    char String[22];
    const unsigned pct = BATTERY_VoltsToPercent(gBatteryVoltageAverage);

    TitleBar(4, "AKKU & INFO");

    sprintf(String, "%u.%02uV", gBatteryVoltageAverage / 100, gBatteryVoltageAverage % 100);
    UI_PrintString(String, 0, 127, 1, 10);

    // Fuellstandsbalken mit Prozentzahl
    UI_DrawRoundRect(4, 27, 99, 36);
    UI_InvertRect(6, 29, 6 + pct * 92 / 100, 34);
    sprintf(String, "%u%%", pct);
    UI_PrintStringSmallNormal(String, 103, 0, 3);

    UI_PrintStringSmallNormal(gChargingWithTypeC ? "Lädt (USB-C)" : "Kein Ladekabel", 0, 127, 5);
    sprintf(String, "%s %s", Version, Edition);
    Tiny(String, 50);
}

// --- Seite "SOS-Ruf" ---------------------------------------------------------
static void DisplaySos(void)
{
    char String[22];

    TitleBar(3, "SOS-RUF");
    if (gQM_Sos >= QM_SOS_START && gQM_Sos <= QM_SOS_LISTEN) {
        sprintf(String, "%u/%u", gQM_SosRound + (gQM_Sos != QM_SOS_LISTEN), QM_SOS_ROUNDS);
        TinyRight(String);
    }

    switch (gQM_Sos) {
        case QM_SOS_ASK:
            BigFreq(gTxVfo->pTX->Frequency);
            UI_PrintStringSmallNormal("sendet SOS-Signal", 0, 127, 3);
            UI_PrintStringSmallBold("NUR IM NOTFALL!", 0, 127, 4);
            Tiny("M = SENDEN     EXIT = NEIN", 48);
            UI_DrawRoundRect(4, 46, 123, 54);
            break;

        case QM_SOS_FORBIDDEN:
            UI_PrintStringSmallBold("Senden geht", 0, 127, 2);
            UI_PrintStringSmallBold("hier nicht!", 0, 127, 3);
            Tiny("FREQUENZ ODER AKKU PRÜFEN", 42);
            Tiny("TASTE = ZURÜCK", 50);
            break;

        default: {  // START, SEND, LISTEN
            UI_PrintString("SOS", 0, 127, 1, 10);
            if (gQM_Sos == QM_SOS_SEND && gQM_SosTone)
                UI_InvertRect(44, 9, 83, 24);   // leuchtet im Takt des Morsetons

            if (gQM_Sos == QM_SOS_LISTEN)
                sprintf(String, "Höre zu ... %us", (gQM_SosTimer + 99) / 100);
            else
                strcpy(String, gQM_Sos == QM_SOS_SEND ? "Sende SOS-Signal" : "Sender startet");
            UI_PrintStringSmallNormal(String, 0, 127, 3);

            const uint32_t f = gTxVfo->pTX->Frequency;
            sprintf(String, "%u.%05u MHz", f / 100000, f % 100000);
            UI_PrintStringSmallNormal(String, 0, 127, 4);
            Tiny(gQM_Sos == QM_SOS_LISTEN ? "PTT = ANTWORTEN  TASTE = STOPP" : "JEDE TASTE = STOPP", 50);
            break;
        }
    }
}

// --- Notfall-Pfeife --------------------------------------------------------
static void DisplayWhistle(void)
{
    TitleBar(3, "Notfall-Pfeife");
    UI_PrintString("PFEIFT", 0, 127, 2, 10);
    UI_PrintStringSmallNormal("Lautstärke: voll", 0, 127, 4);
    Tiny("JEDE TASTE = STOPP", 50);
}

void UI_DisplayQuickMenu(void)
{
    UI_DisplayClear();

    if (gQM_Whistle) {
        DisplayWhistle();
    } else if (gQM_Sos != QM_SOS_OFF) {
        DisplaySos();
    } else {
        switch (gQM_Page) {
            case QM_PAGE_TILES:  DisplayTiles();  break;
            case QM_PAGE_LIST:   DisplayList();   break;
#ifdef ENABLE_FMRADIO
            case QM_PAGE_RADIO:  DisplayRadio();  break;
#endif
            case QM_PAGE_LISTEN: DisplayListen(); break;
            case QM_PAGE_HAM:    DisplayHam();    break;
            default:             DisplayInfo();   break;
        }
    }

    // Akku fast leer: grosser Hinweis, bis eine Taste gedrueckt wird
    if (QUICKMENU_LowBatteryWarning()) {
        memset(gFrameBuffer[1], 0, 6 * 128);   // Zeilen 8..55
        UI_DrawRoundRect(4, 11, 123, 50);
        UI_PrintStringSmallBold("AKKU FAST LEER!", 0, 127, 2);
        UI_PrintStringSmallNormal("Bitte aufladen", 0, 127, 3);
        Tiny("JEDE TASTE = OK", 42);
    }

    // Tastensperre: kurz zeigen, wie man entsperrt
    if (gEeprom.KEY_LOCK && gKeypadLocked) {
        memset(gFrameBuffer[5], 0, 2 * 128);   // Zeilen 40..55
        UI_DrawRoundRect(2, 40, 125, 55);
        Tiny("TASTEN GESPERRT", 42);
        Tiny("TASTE F LANG = ENTSPERREN", 49);
    }

    ST7565_BlitFullScreen();
}

#endif
