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

#include <assert.h>
#include <string.h>

#include "app/chFrScanner.h"
#include "app/dtmf.h"
#ifdef ENABLE_FMRADIO
    #include "app/fm.h"
#endif
#include "driver/keyboard.h"
#include "misc.h"
#ifdef ENABLE_AIRCOPY
    #include "ui/aircopy.h"
#endif
#ifdef ENABLE_FMRADIO
    #include "ui/fmradio.h"
#endif
#ifdef ENABLE_REGA
    #include "app/rega.h"
#endif
#include "ui/inputbox.h"
#ifdef ENABLE_QUICKMENU
    #include "app/quickmenu.h"
    #include "ui/quickmenu.h"
#endif
#include "ui/main.h"
#include "ui/menu.h"
#include "ui/scanner.h"
#include "ui/ui.h"
#include "../misc.h"

GUI_DisplayType_t gScreenToDisplay;
GUI_DisplayType_t gRequestDisplayScreen = DISPLAY_INVALID;

uint8_t           gAskForConfirmation;
bool              gAskToSave;
bool              gAskToDelete;


void (*UI_DisplayFunctions[])(void) = {
#ifdef ENABLE_QUICKMENU
    [DISPLAY_MAIN] = &UI_DisplayQuickMenu,   // SOS-Edition: kein Frequenz-Bildschirm
#else
    [DISPLAY_MAIN] = &UI_DisplayMain,
#endif
    [DISPLAY_MENU] = &UI_DisplayMenu,
#ifdef ENABLE_QUICKMENU
    // SOS-Edition: Radio und Suche laufen im Menue, die alten Bildschirme fallen weg
    [DISPLAY_SCANNER] = &UI_DisplayQuickMenu,
  #ifdef ENABLE_FMRADIO
    [DISPLAY_FM] = &UI_DisplayQuickMenu,
  #endif
#else
    [DISPLAY_SCANNER] = &UI_DisplayScanner,
  #ifdef ENABLE_FMRADIO
    [DISPLAY_FM] = &UI_DisplayFM,
  #endif
#endif

#ifdef ENABLE_AIRCOPY
    [DISPLAY_AIRCOPY] = &UI_DisplayAircopy,
#endif

#ifdef ENABLE_QUICKMENU
    [DISPLAY_QUICKMENU] = &UI_DisplayQuickMenu,
#endif

#ifdef ENABLE_REGA
    [DISPLAY_REGA] = &UI_DisplayREGA,
#endif
};

static_assert(ARRAY_SIZE(UI_DisplayFunctions) == DISPLAY_N_ELEM);

void GUI_DisplayScreen(void)
{
    if (gScreenToDisplay != DISPLAY_INVALID) {
        UI_DisplayFunctions[gScreenToDisplay]();
    }
}

void GUI_SelectNextDisplay(GUI_DisplayType_t Display)
{
    if (Display == DISPLAY_INVALID)
        return;

#ifdef ENABLE_QUICKMENU
    // SOS-Edition: nie der Frequenz-, Radio- oder Scanner-Bildschirm, immer das Schnellwahl-Menue
  #ifdef ENABLE_FMRADIO
    if (Display == DISPLAY_FM && gQM_Page != QM_PAGE_RADIO) {   // Radio wurde eingeschaltet: Radio-Seite
        gQM_Page      = QM_PAGE_RADIO;
        gQM_Category  = 0;
        gScanStateDir = SCAN_OFF;
    }
  #endif
    if (Display == DISPLAY_MAIN || Display == DISPLAY_FM || Display == DISPLAY_SCANNER)
        Display = DISPLAY_QUICKMENU;
#endif

    if (gScreenToDisplay != Display)
    {
        DTMF_clear_input_box();

        gInputBoxIndex       = 0;
        gIsInSubMenu         = false;
        gCssBackgroundScan   = false;
        gScanStateDir        = SCAN_OFF;
        #ifdef ENABLE_FMRADIO
            gFM_ScanState    = FM_SCAN_OFF;
        #endif
        gAskForConfirmation  = 0;
        gAskToSave           = false;
        gAskToDelete         = false;
        gWasFKeyPressed      = false;

        gUpdateStatus        = true;
    }

    gScreenToDisplay = Display;
    gUpdateDisplay   = true;
}
