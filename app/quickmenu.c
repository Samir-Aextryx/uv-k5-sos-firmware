/* Schnellwahl-Menue: sechs Kacheln und einfache Seiten
 *
 *   1 Radio        UKW-Radio, HOCH/RUNTER sucht den naechsten Sender
 *   2 Flugzeug     Flugfunk hoeren (Senden gesperrt)
 *   3 Amateur      Frequenz waehlen (Pfeile oder Zahlen) und mit PTT sprechen
 *   4 SOS          SOS-Ruf senden, PMR/Freenet hoeren und sprechen
 *   5 Werkzeug     Taschenlampe, SOS-Licht, Akku, Tastensperre
 *   6 Einstellung  Energiesparen, Tastentoene, Rauschsperre ...
 *
 * Ueberall: M = OK, EXIT = zurueck, PTT = sprechen (wo erlaubt).
 * Der Frequenz-Bildschirm der Original-Firmware wird nie gezeigt
 * (siehe GUI_SelectNextDisplay in ui/ui.c).
 *
 * Licensed under the Apache License, Version 2.0
 */

#ifdef ENABLE_QUICKMENU

#include <assert.h>
#include <string.h>

#if !defined(ENABLE_TX1750)
    #error "Der SOS-Ruf der Schnellwahl braucht ENABLE_TX1750=1"
#endif

#include "app/action.h"
#include "app/app.h"
#include "app/chFrScanner.h"
#include "app/common.h"
#include "app/generic.h"
#include "app/quickmenu.h"
#ifdef ENABLE_FLASHLIGHT
    #include "app/flashlight.h"
#endif
#ifdef ENABLE_FMRADIO
    #include "app/fm.h"
    #include "driver/bk1080.h"
#endif
#include "audio.h"
#include "driver/backlight.h"
#include "driver/bk4819.h"
#include "frequencies.h"
#include "helper/battery.h"
#include "functions.h"
#include "misc.h"
#include "radio.h"
#include "settings.h"
#include "ui/ui.h"

// ---------------------------------------------------------------------------
// Inhalt. Frequenzen in 10 Hz: 14550000 = 145,500 MHz
// ---------------------------------------------------------------------------
#define SPAN(from, to) (((to) - (from)) / 125)   // Suchlauf-Breite in 1,25 kHz

const QM_Item_t gQM_Items[] = {
    // --- 2 Flugzeug (nur hoeren, AM) ---
    {"Suche 130-136",    13000000, SPAN(13000000, 13600000), QM_LISTEN_AM},
    {"Notfrequenz",      12150000, 0, QM_LISTEN_AM},
    {"Flugfunk 123.45",  12345000, 0, QM_LISTEN_AM},
    // --- 3 Amateur (span = 1: das ganze Band absuchen) ---
    {"Frei wählen",     0,        0, QM_HAM},
    {"2m Anruf",         14550000, 0, QM_HAM},
    {"70cm Anruf",       43350000, 0, QM_HAM},
    {"ISS 145.800",      14580000, 0, QM_HAM},
    {"Suche 2m",         14400000, 1, QM_HAM},
    {"Suche 70cm",       43000000, 1, QM_HAM},
    // --- 4 SOS ---
    {"SOS senden PMR",   44609375, 0, QM_SOS_NFM},
    {"SOS senden 2m",    14550000, 0, QM_SOS},
    {"SOS-Licht",        QM_TOOL_SOS, 0, QM_TOOL},
    {"PMR Kanal 8",      44609375, 0, QM_LISTEN_NFM},
    {"Suche PMR",        44600625, SPAN(44600625, 44619375), QM_LISTEN_NFM},
    {"Suche Freenet",    14902500, SPAN(14902500, 14911250), QM_LISTEN_NFM},
    // --- 5 Werkzeug ---
    {"Taschenlampe",     QM_TOOL_LIGHT, 0, QM_TOOL},
    {"SOS-Licht",        QM_TOOL_SOS,   0, QM_TOOL},
    {"Akku & Info",      QM_TOOL_INFO,  0, QM_TOOL},
    {"Tastensperre",     QM_TOOL_LOCK,  0, QM_TOOL},
    {"Notfall-Pfeife",   QM_TOOL_WHISTLE, 0, QM_TOOL},
    // --- 6 Einstellungen ---
    {"Energiesparen",    QM_SET_SAVE,     0, QM_SET},
    {"Tastentöne",       QM_SET_BEEP,     0, QM_SET},
    {"Rauschsperre",     QM_SET_SQL,      0, QM_SET},
    {"Rauschfilter",     QM_SET_NOISE,    0, QM_SET},
    {"Auto-Sperre",      QM_SET_AUTOLOCK, 0, QM_SET},
    {"Alle Einstell.",   QM_SET_ALL,      0, QM_SET},
};

const QM_Category_t gQM_Categories[] = {
    {"UKW-Radio",     0,  0},
    {"Flugzeug",      0,  3},
    {"Amateurfunk",   3,  6},
    {"SOS",           9,  6},
    {"Werkzeug",      15, 5},
    {"Einstellungen", 20, 6},
};

static_assert(26 == ARRAY_SIZE(gQM_Items), "Kategorien passen nicht zu den Eintraegen");
// jede Liste passt auf einen Bildschirm (6 Zeilen), Direktwahl 1..6

const uint8_t gQM_CategoryCount = ARRAY_SIZE(gQM_Categories);

// Amateurfunk-Baender: 2m und 70cm
static const uint32_t HamBand[2][2] = {{14400000, 14600000}, {43000000, 44000000}};

uint8_t     gQM_Page;
uint8_t     gQM_Category;
uint8_t     gQM_Selected;
const char *gQM_Title;
char        gQM_Input[7];

static bool     listenAm;              // Hoer-Seite: Flugfunk (Senden gesperrt)
static uint32_t listenFrom, listenTo;  // Hoer-Seite: Suchbereich (0 = keine Suche)

uint8_t  gQM_Sos;
uint8_t  gQM_SosRound;
bool     gQM_SosTone;
bool     gQM_Whistle;
uint8_t  gQM_Signal;
bool     gQM_NoiseFilter = true;
uint16_t gQM_SosTimer;
static uint8_t sosPos;   // Element 0..26 in "...---... ...---... ...---..."

// ---------------------------------------------------------------------------

static bool IsAirband(uint32_t f)
{
    return f >= 10800000 && f < 13700000;
}

int8_t QUICKMENU_HamBand(uint32_t f)
{
    for (uint8_t b = 0; b < 2; b++)
        if (f >= HamBand[b][0] && f < HamBand[b][1])
            return b;
    return -1;
}

bool QUICKMENU_HasScan(void)
{
    return listenTo != 0;
}

bool QUICKMENU_CanTalk(void)
{
    return (gQM_Page == QM_PAGE_HAM || (gQM_Page == QM_PAGE_LISTEN && !listenAm))
        && !IsAirband(gTxVfo->pTX->Frequency);
}

// SOS-Ruf beenden. Ist der Sender gerade erst angefordert, die Anforderung
// zuruecknehmen, damit er nicht ohne SOS-Steuerung (Dauerton) startet.
static void SosStop(void)
{
    if (gQM_Sos == QM_SOS_START) {          // Sender noch nicht an: abbrechen
        gFlagPrepareTX = false;
        gAlarmState    = ALARM_STATE_OFF;
    }
    gQM_Sos        = QM_SOS_OFF;
    gUpdateDisplay = true;
}

void ACTION_QuickMenu(void)
{
    if (gCurrentFunction == FUNCTION_TRANSMIT)
        return;

    SosStop();
    GUI_SelectNextDisplay(DISPLAY_QUICKMENU);
}

// Lautsprecher an/aus
#ifdef QM_HOSTTEST
void QM_Speaker(bool on);
#else
static void QM_Speaker(bool on)
{
    if (on)
        AUDIO_AudioPathOn();
    else
        AUDIO_AudioPathOff();
}
#endif

#ifdef ENABLE_FMRADIO
// UKW-Sendersuche: in 0,1-MHz-Schritten weiter, bis ein Sender einrastet.
// Eigener Ablauf im Menue-Takt (unabhaengig vom alten Radio-Bildschirm).
static int8_t   fmSeekDir;      // 0 = keine Suche, +1 hoch, -1 runter
static uint8_t  fmSeekTimer;
static uint16_t fmSeekStart;

static void FmSeekStep(void)
{
    const uint16_t lo = BK1080_GetFreqLoLimit(gEeprom.FM_Band);
    const uint16_t hi = BK1080_GetFreqHiLimit(gEeprom.FM_Band);
    uint16_t f = gEeprom.FM_FrequencyPlaying + fmSeekDir;
    if (f < lo)
        f = hi;
    else if (f > hi)
        f = lo;
    gEeprom.FM_FrequencyPlaying = f;
    BK1080_SetFrequency(f, gEeprom.FM_Band);
    fmSeekTimer = 10;           // 100 ms einschwingen lassen, dann pruefen
}

static void FmSeekStop(void)
{
    fmSeekDir                    = 0;
    gFM_ScanState                = FM_SCAN_OFF;
    gEeprom.FM_SelectedFrequency = gEeprom.FM_FrequencyPlaying;
    SETTINGS_SaveFM();
    QM_Speaker(true);           // Ton wieder an
    gEnableSpeaker = true;
    gUpdateDisplay = true;
}

static void FmSeekStart(int8_t dir)
{
    fmSeekDir     = dir;
    fmSeekStart   = gEeprom.FM_FrequencyPlaying;
    gFM_ScanState = dir;        // Anzeige "Suche Sender ..."
    QM_Speaker(false);          // kein Rauschen waehrend der Suche
    gEnableSpeaker = false;
    BK1080_GetFrequencyDeviation(fmSeekStart);
    FmSeekStep();
}

// Sendersuche von aussen (Seitentaste "Suche", PTT): laeuft sie, anhalten,
// sonst in Richtung dir starten (dir = 0: nur anhalten)
void QUICKMENU_FmSeek(int8_t dir)
{
    if (fmSeekDir)
        FmSeekStop();
    else if (dir && gFmRadioMode)
        FmSeekStart(dir);
}

static void FmSeekTick(void)
{
    if (!fmSeekDir)
        return;
    if (!gFmRadioMode || gQM_Page != QM_PAGE_RADIO || gFM_ScanState == FM_SCAN_OFF) {   // Radio aus: Suche vergessen
        fmSeekDir = 0;
        return;
    }
    if (--fmSeekTimer)
        return;
    const uint16_t f = gEeprom.FM_FrequencyPlaying;
    if (FM_CheckFrequencyLock(f, BK1080_GetFreqLoLimit(gEeprom.FM_Band)) == 0)
        FmSeekStop();                                  // Sender gefunden
    else if (f == fmSeekStart) {
        FmSeekStop();                                  // einmal rum, kein Sender
        gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
    } else
        FmSeekStep();
}
#endif

static void StopScan(void)
{
    if (gScanStateDir != SCAN_OFF)
        CHFRSCANNER_Stop();   // bleibt auf einem gefundenen Sender stehen
#ifdef ENABLE_SCAN_RANGES
    gScanRangeStart = 0;
#endif
}

static void StopEverything(void)
{
    StopScan();
#ifdef ENABLE_FMRADIO
    fmSeekDir = 0;
    if (gFmRadioMode) {
        FM_TurnOff();
        gFlagReconfigureVfos = true;
    }
#endif
    gMonitor = false;
}

// auf eine Frequenz gehen (Frequenz-Modus des passenden Bandes)
static void Tune(uint32_t f, bool am, bool nfm)
{
    StopEverything();

    // immer nur ein Empfaenger: kein Zweikanal-Empfang, kein Cross-Band
    gEeprom.DUAL_WATCH       = DUAL_WATCH_OFF;
    gEeprom.CROSS_BAND_RX_TX = CROSS_BAND_OFF;

    const uint8_t vfo = gEeprom.TX_VFO;
    const uint8_t ch  = FREQUENCY_GetBand(f) + FREQ_CHANNEL_FIRST;

    gEeprom.ScreenChannel[vfo] = ch;
    gEeprom.FreqChannel[vfo]   = ch;
    SETTINGS_SaveVfoIndices();
    RADIO_ConfigureChannel(vfo, VFO_CONFIGURE_RELOAD);

    VFO_Info_t *p = &gEeprom.VfoInfo[vfo];

    p->freq_config_RX.Frequency      = f;
    p->TX_OFFSET_FREQUENCY           = 0;
    p->TX_OFFSET_FREQUENCY_DIRECTION = TX_OFFSET_FREQUENCY_DIRECTION_OFF;
    p->freq_config_RX.CodeType       = CODE_TYPE_OFF;
    p->freq_config_TX.CodeType       = CODE_TYPE_OFF;
    p->Modulation                    = am ? MODULATION_AM : MODULATION_FM;
    p->CHANNEL_BANDWIDTH             = nfm ? BANDWIDTH_NARROW : BANDWIDTH_WIDE;
    p->STEP_SETTING                  = am ? STEP_25kHz : STEP_12_5kHz;
    p->StepFrequency                 = gStepFrequencyTable[p->STEP_SETTING];

    SETTINGS_SaveChannel(ch, vfo, p, 1);
    RADIO_ConfigureChannel(vfo, VFO_CONFIGURE);

    if (nfm) {   // PMR446: kleinste Leistung (nur hier, nicht gespeichert)
        p->OUTPUT_POWER = OUTPUT_POWER_LOW1;
        RADIO_ConfigureSquelchAndOutputPower(p);
    }

    RADIO_SelectVfos();
    RADIO_SetupRegisters(true);
    gUpdateDisplay = true;
}

static void StartScan(uint32_t from, uint32_t to)
{
    // "ScnRev" 0 wuerde die Suche nach dem ersten Funkspruch beenden:
    // dann 3 s nach dem Ende eines Funkspruchs weitersuchen
    if (gEeprom.SCAN_RESUME_MODE == 0)
        gEeprom.SCAN_RESUME_MODE = 12;
#ifdef ENABLE_SCAN_RANGES
    gScanRangeStart = from;
    gScanRangeStop  = to;
#endif
    ACTION_Scan(false);
}

static void HamTune(uint32_t f)
{
    Tune(f, false, false);
    gQM_Page = QM_PAGE_HAM;
}

static void HamScan(int8_t b)
{
    StartScan(HamBand[b][0], HamBand[b][1]);
}

static void OpenListen(const char *title, bool am, uint32_t from, uint32_t to)
{
    gQM_Title  = title;
    listenAm   = am;
    listenFrom = from;
    listenTo   = to;
    gQM_Page   = QM_PAGE_LISTEN;
}

// Rauschsperre 0..9 sofort anwenden (ohne den Kanal neu zu laden)
static void SetSquelch(uint8_t level)
{
    gEeprom.SQUELCH_LEVEL = level;
    RADIO_ConfigureSquelchAndOutputPower(gRxVfo);
    RADIO_SetupRegisters(true);
    gRequestSaveSettings = true;
}

static void OpenRadio(void)
{
#ifdef ENABLE_FMRADIO
    StopEverything();
    gEeprom.FM_IsMrMode         = false;
    gFM_AutoScan                = false;
    gEeprom.FM_FrequencyPlaying = gEeprom.FM_SelectedFrequency;
    ACTION_FM();   // Radio an
    if (gFmRadioMode)
        gQM_Page = QM_PAGE_RADIO;
#else
    gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
#endif
}

// ---------------------------------------------------------------------------
// Werkzeuge und Einstellungen
// ---------------------------------------------------------------------------
const char *QUICKMENU_Value(uint8_t index)
{
    static char num[2];
    const QM_Item_t *it = &gQM_Items[index];
    bool on;

    if (it->type == QM_TOOL) {
        switch (it->f) {
#ifdef ENABLE_FLASHLIGHT
            case QM_TOOL_LIGHT: on = gFlashLightState != FLASHLIGHT_OFF && gFlashLightState != FLASHLIGHT_SOS; break;
            case QM_TOOL_SOS:   on = gFlashLightState == FLASHLIGHT_SOS; break;
#endif
            default: return NULL;
        }
    } else if (it->type == QM_SET) {
        switch (it->f) {
            case QM_SET_SAVE:     on = gEeprom.BACKLIGHT_TIME != 61; break;
            case QM_SET_BEEP:     on = gEeprom.BEEP_CONTROL;         break;
            case QM_SET_AUTOLOCK: on = gEeprom.AUTO_KEYPAD_LOCK;     break;
            case QM_SET_NOISE:    on = gQM_NoiseFilter;              break;
            case QM_SET_SQL:
                num[0] = '0' + gEeprom.SQUELCH_LEVEL;
                return num;
            default:
                return ">";
        }
    } else {
        return NULL;
    }
    return on ? "AN" : "AUS";
}

// --- Notfall-Pfeife: lauter Wechselton (1400/2000 Hz) aus dem Lautsprecher.
// Hoeher geht nicht: der Ton laeuft durch das NF-Filter des Chips (ca. 2,5-3 kHz).
// Es wird nichts gesendet. Jede Taste beendet sie.
#define TONE_REG(hz) ((((uint32_t)(hz) * 1353245u) + (1u << 16)) >> 17)   // wie scale_freq()


static uint8_t whistleTimer, whistleCount;

static void WhistleStart(void)
{
    StopEverything();
    gQM_Whistle  = true;
    whistleTimer = 0;            // im naechsten 10-ms-Schritt geht es los
    whistleCount = 0;
}

static void WhistleStep(void)   // alle 0,2 s: hoch <-> tief
{
    whistleTimer = 20;
    if (whistleCount % 10 == 0) {   // alle 2 s alles neu einstellen (falls ein Piepton dazwischen kam)
        BK4819_SetFilterBandwidth(BK4819_FILTER_BW_WIDE, true);   // breitestes NF-Filter (3 kHz)
        BK4819_PlayTone(1400, false);
        BK4819_WriteRegister(BK4819_REG_70, BK4819_REG_70_ENABLE_TONE1 | (127u << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN));
        QM_Speaker(true);
        BK4819_ExitTxMute();
    }
    BK4819_WriteRegister(BK4819_REG_71, (whistleCount & 1) ? TONE_REG(2000) : TONE_REG(1400));
    whistleCount = (whistleCount + 1) % 20;
}

static void WhistleStop(void)
{
    gQM_Whistle = false;
    BK4819_EnterTxMute();
    QM_Speaker(false);
    BK4819_TurnsOffTones_TurnsOnRX();
    RADIO_SetupRegisters(true);       // Empfaenger wieder normal einstellen
    gUpdateDisplay = true;
}

static void RunTool(uint8_t tool)
{
    switch (tool) {
#ifdef ENABLE_FLASHLIGHT
        case QM_TOOL_LIGHT:   // an <-> aus
        case QM_TOOL_SOS:     // SOS-Blinken an <-> aus
            if (gFlashLightState == FLASHLIGHT_OFF) {
                ACTION_FlashLight();                    // LED an
                if (tool == QM_TOOL_SOS)
                    gFlashLightState = FLASHLIGHT_SOS;
            } else {
                gFlashLightState = FLASHLIGHT_SOS;
                ACTION_FlashLight();                    // aus
            }
            break;
#endif
        case QM_TOOL_INFO:
            gQM_Page = QM_PAGE_INFO;
            break;
        case QM_TOOL_WHISTLE:
            WhistleStart();
            break;
        case QM_TOOL_LOCK:
            COMMON_KeypadLockToggle();
            gKeypadLocked = 4;                          // Hinweis 2 s zeigen
            break;
    }
}

static void ChangeSetting(uint8_t set)
{
    switch (set) {
        case QM_SET_SAVE:      // AN: Licht nach 10 s aus, AUS: Licht immer an
            gEeprom.BACKLIGHT_TIME = (gEeprom.BACKLIGHT_TIME == 61) ? 2 : 61;
            BACKLIGHT_TurnOn();
            break;
        case QM_SET_BEEP:
            gEeprom.BEEP_CONTROL = !gEeprom.BEEP_CONTROL;
            break;
        case QM_SET_SQL:
            SetSquelch((gEeprom.SQUELCH_LEVEL + 1) % 10);
            return;
        case QM_SET_AUTOLOCK:
            gEeprom.AUTO_KEYPAD_LOCK = !gEeprom.AUTO_KEYPAD_LOCK;
            break;
        case QM_SET_NOISE:     // schmaleres Filter: weniger Rauschen (siehe radio.c)
            gQM_NoiseFilter = !gQM_NoiseFilter;
            if (gCurrentFunction != FUNCTION_TRANSMIT)
                RADIO_SetupRegisters(true);
            break;
        default:               // alle Einstellungen: Original-Menue
            StopEverything();
            gFlagRefreshSetting   = true;
            gRequestDisplayScreen = DISPLAY_MENU;
            return;
    }
    gRequestSaveSettings = true;
}

// ---------------------------------------------------------------------------
// SOS-Ruf: sendet das internationale Notzeichen SOS automatisch als Ton
// (... --- ..., der Nutzer muss kein Morsen koennen), dreimal
// hintereinander, dann 15 s Pause zum Hoeren.
// Das Ganze QM_SOS_ROUNDS-mal. Jede Taste beendet es sofort.
// Gesendet wird ueber den 1750-Hz-Tonruf-Mechanismus der Firmware, aber mit
// 800 Hz (siehe functions.c). Ob gesendet werden darf, prueft wie immer
// RADIO_PrepareTX (Band, Akku, AM ...). Flugfunk ist hier zusaetzlich gesperrt.
// ---------------------------------------------------------------------------
#define SOS_UNIT   8      // Laenge eines Punkts: 8 x 10 ms = 80 ms (15 WpM)
#define SOS_LISTEN 1500   // Hoerpause: 15 s

static void SosTone(bool on)
{
    gQM_SosTone = on;
    BK4819_WriteRegister(BK4819_REG_70, on ? (BK4819_REG_70_MASK_ENABLE_TONE1 | (66u << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN)) : 0);
    if (on)
        BK4819_ExitTxMute();
    else
        BK4819_EnterTxMute();
    gUpdateDisplay = true;
}

static void SosStartRound(void)
{
    gQM_Sos        = QM_SOS_START;
    gQM_SosTimer   = 100;                   // 1 s Zeit zum Einschalten des Senders
    gAlarmState    = ALARM_STATE_TX1750;    // Sender ohne Mikrofon, mit Ton
    gFlagPrepareTX = true;
    gUpdateDisplay = true;
}

static void SosBegin(void)
{
    if (IsAirband(gTxVfo->pTX->Frequency)) {   // Flugfunk: niemals senden
        gQM_Sos        = QM_SOS_FORBIDDEN;
        gUpdateDisplay = true;
        return;
    }

    StopEverything();
    gQM_SosRound = 0;
    SosStartRound();
}

// Jemand antwortet auf den SOS-Ruf (oder man drueckt PTT): Hoer-Seite zeigen,
// dort kann man mit PTT sprechen
static void SosAnswer(void)
{
    SosStop();
    OpenListen("SOS-ANTWORT", false, 0, 0);
}

// Hoeren nur auf den Hoer-Seiten, der Amateur-Seite und waehrend des SOS-Rufs.
// Ueberall sonst (Kacheln, Listen, UKW-Radio, Einstellungen) ist der Empfaenger stumm.
bool QUICKMENU_RxAllowed(void)
{
    return gQM_Page == QM_PAGE_LISTEN || gQM_Page == QM_PAGE_HAM || gQM_Sos != QM_SOS_OFF;
}

bool QUICKMENU_LowBatteryWarning(void)
{
    return gLowBattery && !gLowBatteryConfirmed && !gChargingWithTypeC
        && gQM_Sos == QM_SOS_OFF && !gQM_Whistle && gCurrentFunction != FUNCTION_TRANSMIT;
}

void QUICKMENU_TimeSlice10ms(void)
{
    if (!QUICKMENU_RxAllowed() && FUNCTION_IsRx()) {   // laufenden Empfang sofort beenden
        gMonitor = false;
        RADIO_SetupRegisters(true);
        gUpdateDisplay = true;
    }

    if (gQM_Whistle && whistleTimer-- == 0)
        WhistleStep();

#ifdef ENABLE_FMRADIO
    FmSeekTick();
#endif

    // Signalstaerke fuer den Balken: alle 0,25 s messen, nur beim Empfang
    static uint8_t sigTimer;
    uint8_t sig = gQM_Signal;
    if (!FUNCTION_IsRx() || (gQM_Page != QM_PAGE_LISTEN && gQM_Page != QM_PAGE_HAM)) {
        sig      = 0;
        sigTimer = 0;
    } else if (sig == 0 || ++sigTimer >= 25) {
        // S1 = -141 dBm ... S9 = -93 dBm (6 dB je Stufe), 10 = S9 und mehr
        const int16_t s = (BK4819_GetRSSI_dBm() + 147) / 6;
        sig      = (s < 1) ? 1 : (s > 10) ? 10 : s;
        sigTimer = 0;
    }
    if (sig != gQM_Signal) {
        gQM_Signal     = sig;
        gUpdateDisplay = true;
    }

    // Akku-Warnung erscheint oder verschwindet
    static bool lowBat;
    if (QUICKMENU_LowBatteryWarning() != lowBat) {
        lowBat         = !lowBat;
        gUpdateDisplay = true;
    }

    switch (gQM_Sos) {
        case QM_SOS_START:
            if (gCurrentFunction == FUNCTION_TRANSMIT) {
                // Sender ist an, der Ton laeuft schon: das ist der erste Punkt
                gQM_Sos        = QM_SOS_SEND;
                sosPos         = 0;
                gQM_SosTone    = true;
                gQM_SosTimer   = SOS_UNIT;
                gUpdateDisplay = true;
            } else if (gAlarmState == ALARM_STATE_OFF || --gQM_SosTimer == 0) {
                gFlagPrepareTX = false;             // Senden wurde abgelehnt
                gAlarmState    = ALARM_STATE_OFF;
                gQM_Sos        = QM_SOS_FORBIDDEN;
                gUpdateDisplay = true;
            }
            break;

        case QM_SOS_SEND:
            if (gCurrentFunction != FUNCTION_TRANSMIT || gAlarmState == ALARM_STATE_OFF) {
                SosStop();                          // mit einer Taste beendet
                break;
            }
            if (--gQM_SosTimer)
                break;
            if (gQM_SosTone) {
                SosTone(false);
                if (++sosPos == 27) {               // 3x SOS gesendet: Sender aus, hoeren
                    ALARM_Off();
                    FUNCTION_Select(FUNCTION_FOREGROUND);
                    gQM_SosRound++;
                    gQM_Sos      = QM_SOS_LISTEN;
                    gQM_SosTimer = SOS_LISTEN;
                } else {                            // Pause: 1 Einheit, nach SOS 7
                    gQM_SosTimer = SOS_UNIT * (sosPos % 9 ? 1 : 7);
                }
            } else {                                // Punkt = 1, Strich = 3 Einheiten
                SosTone(true);
                gQM_SosTimer = SOS_UNIT * ((uint8_t)(sosPos % 9 - 3) < 3 ? 3 : 1);
            }
            break;

        case QM_SOS_LISTEN:
            if (gScreenToDisplay != DISPLAY_QUICKMENU || gCurrentFunction == FUNCTION_TRANSMIT) {
                SosStop();                          // Menue verlassen oder selbst gesendet
            } else if (gCurrentFunction == FUNCTION_RECEIVE) {
                SosAnswer();                        // jemand antwortet
            } else if (--gQM_SosTimer == 0) {
                if (gQM_SosRound >= QM_SOS_ROUNDS) {
                    SosStop();
                    gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;
                } else {
                    SosStartRound();
                }
            } else if (gQM_SosTimer % 100 == 0) {
                gUpdateDisplay = true;              // Sekunden-Anzeige
            }
            break;
    }

    if (gScreenToDisplay != DISPLAY_QUICKMENU)
        return;

    // Anzeige neu zeichnen, wenn sich Empfang, Frequenz oder Suche aendern
    static uint32_t last;
    uint32_t now = gRxVfo->freq_config_RX.Frequency ^ ((uint32_t)gCurrentFunction << 27)
                 ^ ((uint32_t)(gScanStateDir != SCAN_OFF) << 31) ^ ((uint32_t)(gKeypadLocked != 0) << 30);
#ifdef ENABLE_FMRADIO
    now ^= gEeprom.FM_FrequencyPlaying ^ ((uint32_t)(gFM_ScanState != FM_SCAN_OFF) << 26);
#endif
    if (now != last) {
        last           = now;
        gUpdateDisplay = true;
    }
}

// ---------------------------------------------------------------------------
// Tasten
// ---------------------------------------------------------------------------
static void SelectItem(uint8_t index)
{
    const QM_Item_t *it = &gQM_Items[index];
    const bool nfm = it->type == QM_LISTEN_NFM || it->type == QM_SOS_NFM;

    switch (it->type) {
        case QM_TOOL:
            RunTool(it->f);
            break;

        case QM_SET:
            ChangeSetting(it->f);
            break;

        case QM_SOS:
        case QM_SOS_NFM:
            Tune(it->f, false, nfm);     // erst auf die Frequenz gehen,
            gQM_Sos = QM_SOS_ASK;        // dann nachfragen
            break;

        case QM_HAM: {
            uint32_t f = it->f;
            if (f == 0) {                // frei waehlen: letzte Amateurfunk-Frequenz
                f = gTxVfo->freq_config_RX.Frequency;
                if (QUICKMENU_HamBand(f) < 0)
                    f = 14550000;
            }
            HamTune(f);
            if (it->span)
                HamScan(QUICKMENU_HamBand(f));
            break;
        }

        default: {                       // hoeren
            const bool     am = it->type == QM_LISTEN_AM;
            const uint32_t to = it->span ? it->f + it->span * 125u : 0;
            Tune(it->f, am, nfm);
            OpenListen(it->name, am, it->f, to);
            if (to)
                StartScan(it->f, to);
            break;
        }
    }
}

static void Open(uint8_t sel)
{
    if (gQM_Page == QM_PAGE_TILES) {
        gQM_Category = sel;
        if (gQM_Categories[sel].count == 0) {
            OpenRadio();
        } else {
            gQM_Page     = QM_PAGE_LIST;
            gQM_Selected = 0;
        }
        return;
    }

    gQM_Selected = sel;
    SelectItem(gQM_Categories[gQM_Category].first + sel);
}

static void BackToTiles(void)
{
    gQM_Page     = QM_PAGE_TILES;
    gQM_Selected = gQM_Category;
}

// Amateurfunk-Seite: Ziffern tippen, nach 6 Ziffern einstellen ("145500")
static void HamDigit(uint8_t d)
{
    uint8_t n = strlen(gQM_Input);

    StopScan();
    gQM_Input[n++] = '0' + d;
    gQM_Input[n]   = 0;
    if (n < 6)
        return;

    uint32_t f = 0;
    for (uint8_t i = 0; i < 6; i++)
        f = f * 10 + gQM_Input[i] - '0';
    f *= 100;                                  // kHz -> 10 Hz
    gQM_Input[0] = 0;

    if (QUICKMENU_HamBand(f) < 0) {
        gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;   // nicht im 2m/70cm-Band
        return;
    }
    HamTune(f);
}

static void HamKeys(KEY_Code_t Key)
{
    uint32_t     f = gTxVfo->freq_config_RX.Frequency;
    const int8_t b = QUICKMENU_HamBand(f);

    switch (Key) {
        case KEY_0 ... KEY_9:
            HamDigit(Key - KEY_0);
            break;

        case KEY_UP:
        case KEY_DOWN:
            if (gQM_Input[0] || b < 0)
                break;
            StopScan();
            f = gTxVfo->freq_config_RX.Frequency;       // Suche kann sie geaendert haben
            f = (Key == KEY_UP) ? f + 1250 : f - 1250;  // 12,5-kHz-Schritte
            if (f >= HamBand[b][1])
                f = HamBand[b][0];
            else if (f < HamBand[b][0])
                f = HamBand[b][1] - 1250;
            HamTune(f);
            break;

        case KEY_MENU:   // Suche an/aus
            if (gScanStateDir != SCAN_OFF)
                StopScan();
            else if (b >= 0 && !gQM_Input[0])
                HamScan(b);
            break;

        case KEY_STAR: { // Sendeleistung: niedrig -> mittel -> hoch
            const uint8_t p = gTxVfo->OUTPUT_POWER;
            gTxVfo->OUTPUT_POWER = (p >= OUTPUT_POWER_HIGH) ? OUTPUT_POWER_LOW1 : (p >= OUTPUT_POWER_MID) ? OUTPUT_POWER_HIGH : OUTPUT_POWER_MID;
            gRequestSaveChannel  = 1;
            break;
        }

        case KEY_EXIT:
            if (gQM_Input[0]) {                       // letzte Ziffer loeschen
                gQM_Input[strlen(gQM_Input) - 1] = 0;
                break;
            }
            StopEverything();
            gQM_Page = QM_PAGE_LIST;
            break;

        default:
            gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
            break;
    }
}

void QUICKMENU_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld)
{
    // Notfall-Pfeife: jede Taste (auch PTT) beendet sie, sonst passiert nichts
    if (gQM_Whistle) {
        if (bKeyPressed && !bKeyHeld)
            WhistleStop();
        return;
    }

    // Hinweis "Akku fast leer": jede Taste bestaetigt ihn (PTT funktioniert weiter)
    if (QUICKMENU_LowBatteryWarning() && Key != KEY_PTT) {
        if (bKeyPressed && !bKeyHeld) {
            gLowBatteryConfirmed = true;
            gUpdateDisplay       = true;
            gBeepToPlay          = BEEP_1KHZ_60MS_OPTIONAL;
        }
        return;
    }

    // SOS-Ruf: M in der Abfrage startet - erst beim Loslassen. Sonst wuerde
    // das Loslassen einer lange gedrueckten Taste den Ruf (wie beim 1750-Hz-
    // Ton) sofort wieder beenden.
    static bool sosMenuDown;
    if (gQM_Sos == QM_SOS_ASK && Key == KEY_MENU) {
        if (bKeyPressed && !bKeyHeld)
            sosMenuDown = true;             // nur ein neuer Druck in der Abfrage zaehlt
        else if (sosMenuDown) {
            sosMenuDown = false;
            SosBegin();
        }
        return;
    }
    sosMenuDown = false;

    // jede andere Taste beendet den SOS-Ruf; PTT in der Hoerpause = antworten
    if (gQM_Sos != QM_SOS_OFF && bKeyPressed && !bKeyHeld) {
        if (Key == KEY_PTT && gQM_Sos == QM_SOS_LISTEN) {
            SosAnswer();
        } else {
            SosStop();
            gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;
            if (Key != KEY_PTT)
                return;
        }
    }

    if (Key == KEY_PTT) {
        if (!bKeyPressed || QUICKMENU_CanTalk())
            GENERIC_Key_PTT(bKeyPressed);
        else if (!bKeyHeld)
            gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;   // hier nicht senden
        return;
    }

    if (Key == KEY_F) {                     // F lang = Tastensperre
        GENERIC_Key_F(bKeyPressed, bKeyHeld);
        return;
    }

    // HOCH/RUNTER auch beim Festhalten (Wiederholung), alles andere beim Druecken
    if (!bKeyPressed || (bKeyHeld && Key != KEY_UP && Key != KEY_DOWN))
        return;

    gBeepToPlay    = BEEP_1KHZ_60MS_OPTIONAL;
    gUpdateDisplay = true;

    switch (gQM_Page) {
        case QM_PAGE_TILES:
        case QM_PAGE_LIST: {
            const uint8_t count = (gQM_Page == QM_PAGE_TILES) ? gQM_CategoryCount : gQM_Categories[gQM_Category].count;
            switch (Key) {
                case KEY_UP:
                    gQM_Selected = (gQM_Selected == 0) ? count - 1 : gQM_Selected - 1;
                    break;
                case KEY_DOWN:
                    gQM_Selected = (gQM_Selected + 1 >= count) ? 0 : gQM_Selected + 1;
                    break;
                case KEY_1 ... KEY_9:
                    if (Key - KEY_1 < count)
                        Open(Key - KEY_1);
                    else
                        gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
                    break;
                case KEY_MENU:
                    Open(gQM_Selected);
                    break;
                case KEY_EXIT:
                    if (gQM_Page == QM_PAGE_LIST)
                        BackToTiles();
                    break;
                default:
                    gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
                    break;
            }
            break;
        }

        case QM_PAGE_RADIO:
#ifdef ENABLE_FMRADIO
            if (Key == KEY_UP || Key == KEY_DOWN) {   // naechsten Sender suchen
                if (!bKeyHeld) {
                    QUICKMENU_FmSeek(Key == KEY_UP ? 1 : -1);   // nochmal druecken = anhalten
                }
                break;
            }
#endif
            if (Key == KEY_EXIT) {                    // Radio aus
                StopEverything();
                BackToTiles();
            } else {
                gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
            }
            break;

        case QM_PAGE_HAM:
            HamKeys(Key);
            break;

        case QM_PAGE_LISTEN:
            if (Key == KEY_EXIT) {
                StopEverything();
                gQM_Page = QM_PAGE_LIST;
            } else if (Key == KEY_UP && gEeprom.SQUELCH_LEVEL < 9) {   // Rauschsperre
                SetSquelch(gEeprom.SQUELCH_LEVEL + 1);
            } else if (Key == KEY_DOWN && gEeprom.SQUELCH_LEVEL > 0) {
                SetSquelch(gEeprom.SQUELCH_LEVEL - 1);
            } else if (Key == KEY_MENU && listenTo && !bKeyHeld) {     // Suche an/aus
                if (gScanStateDir != SCAN_OFF)
                    StopScan();                // bleibt auf dem letzten Funkspruch
                else
                    StartScan(listenFrom, listenTo);
            } else if (!bKeyHeld) {
                gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
            }
            break;

        default:                                       // Akku & Info: jede Taste
            gQM_Page = QM_PAGE_LIST;
            break;
    }
}

#endif
