/* Schnellwahl-Menue: sechs Kacheln (Radio, Flugzeug, Amateur, SOS,
 * Werkzeug, Einstellungen) und einfache Seiten zum Hoeren und Sprechen.
 *
 * Licensed under the Apache License, Version 2.0
 */

#ifndef APP_QUICKMENU_H
#define APP_QUICKMENU_H

#ifdef ENABLE_QUICKMENU

#include <stdbool.h>
#include <stdint.h>

#include "driver/keyboard.h"

// Art eines Listeneintrags
enum {
    QM_LISTEN_AM = 0, // nur hoeren, AM (Flugfunk, Senden gesperrt); span = Suchlauf
    QM_LISTEN_NFM,    // hoeren, FM schmal (PMR/Freenet); PTT erlaubt
    QM_HAM,           // Amateurfunk-Seite (Frequenz waehlen, sprechen); f = 0: frei
    QM_SOS,           // SOS-Ruf (Morse-Ton) auf f, FM breit
    QM_SOS_NFM,       // SOS-Ruf auf f, FM schmal, kleinste Leistung (PMR446)
    QM_TOOL,          // Werkzeug, f = QM_TOOL_*
    QM_SET,           // Einstellung, f = QM_SET_*
};

// Seiten
enum {
    QM_PAGE_TILES = 0,  // sechs Kacheln (Startseite)
    QM_PAGE_LIST,       // Liste einer Kachel
    QM_PAGE_RADIO,      // UKW-Radio
    QM_PAGE_LISTEN,     // Hoeren (Flugfunk, PMR ...)
    QM_PAGE_HAM,        // Amateurfunk: Frequenz waehlen und sprechen
    QM_PAGE_INFO,       // Akku & Info
};

// Ablauf des SOS-Rufs
enum {
    QM_SOS_OFF = 0,
    QM_SOS_ASK,        // Sicherheitsabfrage "Wirklich senden?"
    QM_SOS_START,      // Sender wird eingeschaltet
    QM_SOS_SEND,       // sendet ...---... (3x)
    QM_SOS_LISTEN,     // Pause zum Hoeren, ob jemand antwortet
    QM_SOS_FORBIDDEN,  // auf dieser Frequenz darf nicht gesendet werden
};

#define QM_SOS_ROUNDS 10   // so oft wird gesendet, dann ist Schluss

enum {
    QM_TOOL_LIGHT = 0,  // Taschenlampe an/aus
    QM_TOOL_SOS,        // SOS-Licht an/aus
    QM_TOOL_INFO,       // Akku & Info
    QM_TOOL_LOCK,       // Tastensperre
    QM_TOOL_WHISTLE,    // Notfall-Pfeife (lauter Ton aus dem Lautsprecher)
};

enum {
    QM_SET_SAVE = 0,    // Energiesparen: Licht nach 10 s aus / immer an
    QM_SET_BEEP,        // Tastentoene
    QM_SET_SQL,         // Rauschsperre 0..9
    QM_SET_AUTOLOCK,    // automatische Tastensperre
    QM_SET_NOISE,       // Rauschfilter an/aus
    QM_SET_ALL,         // alle Einstellungen (Original-Menue)
};

typedef struct {
    const char *name;   // max. 16 Zeichen
    uint32_t    f;      // in 10 Hz (14550000 = 145,500 MHz)
    uint16_t    span;   // Breite des Suchlaufs in 1,25-kHz-Schritten, sonst 0
    uint8_t     type;
} QM_Item_t;

typedef struct {
    const char *name;
    uint8_t     first;
    uint8_t     count;   // 0 = keine Liste (Radio)
} QM_Category_t;

extern const QM_Item_t     gQM_Items[];
extern const QM_Category_t gQM_Categories[];
extern const uint8_t       gQM_CategoryCount;
extern const uint8_t       gQM_Icons[][8];

extern uint8_t gQM_Page;       // QM_PAGE_*
extern uint8_t gQM_Category;   // gewaehlte Kachel
extern uint8_t gQM_Selected;   // Auswahl auf der Kachelseite / in der Liste
extern const char *gQM_Title;  // Titel der Hoer-Seite
extern char    gQM_Input[7];   // Frequenz-Eingabe auf der Amateurfunk-Seite ("145500")

extern uint8_t  gQM_Sos;        // QM_SOS_*
extern uint8_t  gQM_SosRound;   // gesendete Runden
extern bool     gQM_SosTone;    // Ton ist gerade an
extern uint16_t gQM_SosTimer;   // 10-ms-Schritte bis zum naechsten Wechsel

extern bool     gQM_Whistle;     // Notfall-Pfeife laeuft
extern uint8_t  gQM_Signal;      // Signalstaerke 0 (kein Empfang), 1..10
extern bool     gQM_NoiseFilter; // Rauschfilter (schmaleres Empfangsfilter)

const char *QUICKMENU_Value(uint8_t index);   // Wert rechts in der Liste (oder NULL)
bool        QUICKMENU_HasScan(void);          // Hoer-Seite hat einen Suchbereich
bool        QUICKMENU_CanTalk(void);          // PTT erlaubt (Hoer-/Amateurseite)
int8_t      QUICKMENU_HamBand(uint32_t f);    // 0 = 2m, 1 = 70cm, -1 = kein Amateurfunk-Band

bool QUICKMENU_RxAllowed(void);
#ifdef ENABLE_FMRADIO
void QUICKMENU_FmSeek(int8_t dir);
#endif
bool QUICKMENU_LowBatteryWarning(void);   // Hinweis "Akku fast leer" zeigen
void QUICKMENU_TimeSlice10ms(void);

void ACTION_QuickMenu(void);
void QUICKMENU_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);

#endif
#endif
