# UV-K5(8): F4HWN-Firmware „SOS-Edition“ – ganz einfach zu bedienen

Grundlage ist die F4HWN-Firmware, Stand `fbcf26d` vom 2. Dezember 2025. Diese Edition ersetzt die normale Bedienung durch ein einfaches Kachel-Menü:

* **6 Kacheln:** Radio, Flugzeug, Amateur, SOS, Werkzeug, Einstellungen,
* **M = OK** überall, **EXIT = zurück**, **PTT = sprechen** (wo erlaubt),
* **kein Frequenz-Bildschirm** mehr: Jede Funktion hat eine eigene, einfache Seite,
* **SOS-Ruf:** Das Funkgerät piept das internationale Notzeichen SOS von selbst. Du musst kein Morsen können.
* **echte Umlaute** (ä ö ü Ä Ö Ü ß) auf dem Display,
* **Schneller Start:** Kein Warten: Das Samir-Startbild (ohne Animation) steht nur, solange das Funkgerät startet, dann kommt sofort das Menü. Das Display ist sofort voll hell (kein langsames Aufhellen).
* **Akku-Warnung:** Ist der Akku fast leer, erscheint groß „AKKU FAST LEER!“ und es piept. Jede Taste bestätigt den Hinweis (PTT geht trotzdem).

**Nur für UV-K5(8) / UV-K6 / UV-5R Plus mit DP32G030-Chip.** Nicht für UV-K5 V3 oder UV-K1 verwenden.

## Datei

`compiled-firmware/f4hwn.sos.packed.bin` – die neue, einfache Version.

`compiled-firmware/f4hwn.sos-spektrum.packed.bin` ist eine **alte** Version (altes Menü, mit Spektrum). Sie wird nicht mehr gepflegt.

Abgeschaltet sind: VOX (Sprachsteuerung), Schlafmodus, Sende- und Empfangs-Timer, Mikrofon-Pegelbalken, NARROWER, Kanal in VFO kopieren, CA, Invertieren, Kontrast, Fortsetzen nach dem Einschalten, Sperrmodus „PMR“, AirCopy, GMRS/FRS/MURS, Screenshot. Programmieren und Sichern mit CHIRP geht weiterhin.

## Bedienung

Nach dem Einschalten erscheinen die **6 Kacheln**.

* `HOCH`/`RUNTER` wählt eine Kachel, **`M`** öffnet sie. Oder einfach die Ziffer `1`–`6` drücken.
* In jeder Liste genauso: `HOCH`/`RUNTER` + `M` oder die Ziffer.
* **`EXIT`** geht immer einen Schritt zurück.
* **Gehört wird nur auf den Hör-Seiten** (Flugzeug, PMR, Amateur) und während des SOS-Rufs. Mit `EXIT` verlässt du die Seite, dann ist sofort Ruhe. In den Kacheln, Listen und beim UKW-Radio ist der Funkempfang stumm.
* Unten auf jeder Seite steht, was die Tasten gerade tun.

### 1 Radio

Das UKW-Radio spielt sofort den zuletzt gehörten Sender.

* `HOCH` / `RUNTER`: nächsten Sender suchen. Die Suche geht in 0,1-MHz-Schritten, und hält beim nächsten gut empfangbaren Sender an (ganzes Band: etwa 20 Sekunden). Nochmal drücken hält die Suche an. Ohne Sender im ganzen Band endet sie mit einem Doppel-Piep.
* Lautstärke: Drehknopf oben,
* `EXIT`: Radio aus, zurück zu den Kacheln.

### 2 Flugzeug (nur hören)

| Eintrag | Was passiert |
|---|---|
| Suche 130-136 | sucht 130,000–136,000 MHz in 25-kHz-Schritten ab (AM). Ein Durchlauf dauert etwa 22 Sekunden. |
| Notfrequenz | 121,500 MHz, Luftfahrt-Notfrequenz (AM) |
| Flugfunk 123.45 | 123,450 MHz, Luft-Luft-Frequenz (AM) |

Die Seite zeigt groß die Frequenz, oben rechts **AM** und darunter **EMPFANG**, sobald jemand spricht. Daneben zeigen **10 Balken die Signalstärke**: Je mehr Balken, desto stärker (und meist näher) ist der Sender. Senden ist hier gesperrt.

* Die Suche hält bei einem Funkspruch an und sucht **3 Sekunden nach dessen Ende** von selbst weiter.
* `HOCH` / `RUNTER`: **Rauschsperre** (0–9). Hörst du nichts, stell sie niedriger. Rauscht es dauernd, stell sie höher.
* `M`: Suche anhalten (bleibt auf der aktuellen Frequenz) / wieder starten.
* Flugfunk hört man am besten draußen oder am Fenster. In der Nähe eines Flughafens ist am meisten los.

### 3 Amateur (selbst Frequenz wählen und sprechen)

| Eintrag | Was passiert |
|---|---|
| Frei wählen | letzte Amateurfunk-Frequenz (sonst 145,500) |
| 2m Anruf | 145,500 MHz |
| 70cm Anruf | 433,500 MHz |
| ISS 145.800 | Sprechfunk der Raumstation ISS (nur wenn sie über dir ist) |
| Suche 2m | sucht 144–146 MHz ab |
| Suche 70cm | sucht 430–440 MHz ab |

Auf der Amateurfunk-Seite:

* `HOCH` / `RUNTER`: Frequenz in 12,5-kHz-Schritten ändern (festhalten = schnell),
* **Ziffern tippen:** 6 Ziffern, z. B. `1 4 5 5 0 0` = 145,500 MHz. `EXIT` löscht die letzte Ziffer. Erlaubt sind 144–146 und 430–440 MHz.
* **`PTT` gedrückt halten = sprechen**, loslassen = hören,
* `M`: Suche an/aus (hält bei einem Signal an),
* `*`: Sendeleistung niedrig → mittel → hoch,
* `EXIT`: zurück zur Liste.

### 4 SOS

| Eintrag | Was passiert |
|---|---|
| SOS senden PMR | SOS-Ruf auf PMR-Kanal 8 (446,09375 MHz, kleinste Leistung) |
| SOS senden 2m | SOS-Ruf auf 145,500 MHz |
| SOS-Licht | Taschenlampe blinkt SOS (nochmal `M` = aus) |
| PMR Kanal 8 | hören und mit `PTT` sprechen |
| Suche PMR | sucht alle 16 PMR-Kanäle ab |
| Suche Freenet | sucht die Freenet-Kanäle ab |

**So funktioniert der SOS-Ruf:**

1. Einen SOS-Eintrag wählen. Es erscheint „NUR IM NOTFALL!“.
2. **`M` drücken** = senden. `EXIT` = nein.
3. Das Funkgerät piept dreimal SOS (· · · – – – · · ·). Das macht es ganz allein.
4. Dann hört es **15 Sekunden**, ob jemand antwortet. Antwortet jemand, hört der Ruf auf und du siehst die Hör-Seite. Mit **`PTT`** sprichst du (z. B. „Mayday, Mayday, Mayday, hier ist …, ich bin in …“). Du kannst auch in der Pause selbst `PTT` drücken.
5. Das wiederholt sich höchstens 10-mal (etwa 3,5 Minuten). **Jede Taste beendet den SOS-Ruf sofort.**

Auf Flugfunk-Frequenzen sendet der SOS-Ruf nie. Geht Senden nicht (Akku leer, Band gesperrt), steht dort „Senden geht hier nicht!“.

### 5 Werkzeug

| Eintrag | Was passiert |
|---|---|
| Taschenlampe | an / aus (rechts steht AN oder AUS) |
| SOS-Licht | Lampe blinkt SOS, an / aus |
| Akku & Info | Spannung, Prozent, Ladekabel, Version |
| Tastensperre | sperrt die Tasten. **Entsperren: Taste `F` lang drücken** |
| Notfall-Pfeife | lauter Pfeifton (1400/2000 Hz im Wechsel) aus dem Lautsprecher, damit dich jemand in der Nähe findet. Es wird **nichts gesendet**. Lautstärke-Drehknopf ganz aufdrehen. **Jede Taste stoppt die Pfeife.** |

### 6 Einstellungen

`M` schaltet den Wert rechts um.

| Eintrag | Werte |
|---|---|
| Energiesparen | **AN** = Bildschirm geht nach 10 Sekunden aus (eine Taste macht ihn wieder hell). **AUS** = Bildschirm bleibt immer hell. |
| Tastentöne | Piepen bei Tastendruck an/aus |
| Rauschsperre | 0–9: Je höher, desto stärker muss ein Signal sein, damit man es hört. 0 = immer Rauschen. |
| Rauschfilter | **AN** (Standard): Flugfunk (AM) wird mit einem schmaleren Filter empfangen. Das lässt viel weniger Rauschen durch. Bei FM wird das Filter bei schwachen Signalen von selbst schmaler. **AUS**: Filter wie in der Original-Firmware. |
| Auto-Sperre | Tasten sperren sich nach einiger Zeit von selbst |
| Alle Einstell. | öffnet das vollständige Original-Menü der F4HWN-Firmware. `EXIT` führt zurück. |

**ISS-Tipp:** Wann die ISS über dich fliegt, zeigen Apps wie „ISS Detector“ oder die Seite heavens-above.com. Ein Überflug dauert etwa 10 Minuten.

## Rechtliches (Deutschland)

* **Flugfunk und Luftnot:** nur Empfang, Senden ist gesperrt. Was du hörst, darfst du nicht weitergeben (§ 5 TDDDG).
* **Amateurfunk:** Senden nur mit Amateurfunkzeugnis und Rufzeichen. Hören darf jeder.
* **PMR446 und Freenet:** Das UV-K5 hat dafür keine Zulassung. Senden ist nur in einem echten Notfall bei Gefahr für Leib und Leben vertretbar.
* **SOS-Ruf:** nur in echter Not benutzen. Ein falscher Notruf kann bestraft werden (§ 145 StGB) und blockiert Funkkanäle für andere.
* **Polizei, Feuerwehr und Rettungsdienst** funken digital und verschlüsselt (TETRA). Das UV-K5 kann das nicht empfangen.

## Flashen

1. **Backup machen.** Die Kalibrierdaten sind für jedes Gerät einzeln. In CHIRP mit „Download from radio“ ein Abbild ziehen und speichern.
2. Funkgerät ausschalten, **PTT gedrückt halten und einschalten**. Die Taschenlampen-LED leuchtet, das Display bleibt leer: Das ist der Bootloader-Modus.
3. Programmierkabel (K-Stecker) anschließen.
4. In Chrome oder Edge <https://egzumer.github.io/uvtools/> öffnen → **Firmware flasher** → `f4hwn.sos.packed.bin` auswählen → **Flash**.
5. Neu starten. Das Samir-Logo erscheint, danach die Kacheln.

Zurück geht es jederzeit: einfach eine andere Firmware auf demselben Weg flashen.

## Selbst bauen

Dafür brauchst du Docker Desktop. Befehle in Git Bash:

```sh
docker build -t uvk5-build .
./build-sos.sh
```

Was im Quellcode geändert ist:

* **Menü:** `app/quickmenu.c` (Logik, Liste aller Einträge – Frequenzen hier ändern) und `ui/quickmenu.c` (Anzeige, Symbole).
* **Kein Frequenz-Bildschirm:** `ui/ui.c` leitet jede Anfrage nach dem Hauptbildschirm, dem alten UKW-Radio-Bildschirm und dem Ton-Scanner auf das Menü um (Radio → einfache Radio-Seite). In den Tabellen in `ui/ui.c` und `app/app.c` zeigen `DISPLAY_MAIN`, `DISPLAY_FM` und `DISPLAY_SCANNER` auf das Menü, deshalb fällt deren alter Code beim Bauen weg (spart etwa 8 KB).
* **Logo:** `ui/welcome.c` (`ENABLE_SAMIR_LOGO`), festes Bild; die Startzeit steht in `main.c` (`boot_counter_10ms = 0`); das langsame Aufhellen beim Start ist in `driver/backlight.c` abgeschaltet.
* **UKW-Sendersuche:** eigener Ablauf in `app/quickmenu.c` (`FmSeekStart`/`FmSeekTick`, prüft mit `FM_CheckFrequencyLock`). Die alte Suche (`FM_Play` in `app/app.c`, `ACTION_Scan_FM` in `app/action.c`) ist in der SOS-Edition abgeschaltet; die Seitentaste „Suche“ und PTT nutzen die neue.
* **SOS-Ruf:** Takt in `QUICKMENU_TimeSlice10ms` (`app/quickmenu.c`), 800-Hz-Ton in `functions.c`, `ALARM_Off` in `app/app.c` ist dafür nicht mehr `static`.
* **`app/app.c`:** Eine VFO-Neukonfiguration schließt das Menü nicht. Im Menü wird (ohne laufenden SOS-Ruf) Strom gespart. Die automatische Tastensperre funktioniert auch im Menü.
* **Umlaute:** Zeichen in `font.c` (`gFontSmallUml`, `gFontSmallBoldUml`, `gFont3x5Uml`). `ui/helper.c` erkennt ä ö ü Ä Ö Ü ß im Text (UTF-8) und zählt sie beim Zentrieren als ein Zeichen (`UI_StrLen`). Texte im Quellcode einfach mit Umlauten schreiben. Die große Schrift hat keine Umlaute.
* **Einbindung:** `ui/ui.*`, `app/action.c`, `ui/menu.c`, `ui/helper.*`, `settings.h`, `Makefile` (`ENABLE_QUICKMENU`, `ENABLE_SAMIR_LOGO`).
