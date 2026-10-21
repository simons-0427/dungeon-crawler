# Testbericht – Dungeon Crawler Android 1.5.0

Stand: 13.08.2026

Finales Artefakt: `DungeonCrawler-Android-v1.5.0.apk`  
Größe: `35.590.839` Byte  
SHA-256: `79B08D568BC30F4169E61EDC1969448BB1EB257D7CBDCBEDAA90E14E0D20FF69`

## Erfolgreich geprüft

- Offline-Build mit Gradle 8.14 und Android Gradle Plugin 8.13.0
- C99/NDK-Kompilierung ohne Compilerwarnungen für `arm64-v8a` und `x86_64`
- Android Lint erfolgreich: 0 Fehler; 8 nicht blockierende Warnungen zu SDK-Ziel,
  fester Querformat-Ausrichtung und bestehenden Launcher-Ressourcen
- sichtbarer Back-Button und Eingabeprüfung verwenden dieselbe zentrale
  `170 × 52`-Touchfläche einschließlich der Android-Sicherheitsränder
- Back-Navigation reagiert in allen sechs Unterseiten sowohl auf die normale
  Maus-/Touch-Abbildung als auch direkt auf einen neu aufgesetzten Finger
- veraltete `130 × 40`-Trefferfläche und zustandsändernde Klicklogik während des
  Zeichnens entfernt
- Desktop-Hinweise `[E] Enemies` und `[F11] Fullscreen` vollständig aus Hauptmenü,
  Tutorial, Einstellungen und Spiel-HUD entfernt; die Desktop-Hotkeys selbst bleiben
  unverändert funktionsfähig
- oberes Spiel-HUD auf kompakte `88 px` Höhe reduziert; seine nochmals vergrößerten
  Inhalte beginnen mit höchstens `2 px` Abstand zur Android-Sicherheitskante
- Score-, Münz-, Schatz-, Gegner-, Item- und Bestwert-Anzeigen verwenden größere Symbole
  und Schriften; die Kill-Kugel ist ohne Lichtreflex flach wie das Bestwert-Symbol
- bis zu fünf Inventarplätze auf jeweils `86 × 86 px` vergrößert; Darstellung und
  Touch-Abfrage verwenden dieselben Rechtecke
- Pause-Button auf `72 × 72 px` vergrößert; Pausenfenster responsiv bis
  `820 × 500 px`, beide Menüschaltflächen jeweils `72 px` hoch
- `TREASURE FOUND!` und `PORTAL USED!` von `52 px` auf `68 px`, `YOU DIED` von
  `64 px` auf `72 px`, `Coins / Kills` auf `36 px` und `NEXT ROUND` auf `28 px`
  vergrößert
- gesamter `NEXT ROUND`-Button um `26 px` nach unten verschoben, sodass er klaren
  Abstand zur vergrößerten Ergebniszeile besitzt; sichtbares Rechteck und Touchfläche
  verwenden weiterhin dieselbe zentrale Layoutfunktion
- eigene `FullscreenNativeActivity` erweitert `NativeActivity`; das gepackte Manifest
  setzt `hasCode=true`, diese Activity als Launcher und `android:immersive=true`
- Android 11 und neuer blenden Status-, Navigations- und Caption-Bar mit
  `WindowInsetsController.hide(systemBars())` aus und verwenden vorübergehende
  Systemleisten per Randgeste; Android 8 bis 10 nutzt `IMMERSIVE_STICKY`
- Immersive Fullscreen wird in `onCreate`, `onResume` und nach Fokusgewinn erneut
  aktiviert; Display-Cutouts werden an den kurzen Bildschirmseiten zugelassen und
  weiterhin durch die vorhandenen Safe Insets geschützt
- Android-Fullscreen ist in den Spieleinstellungen fest auf `ON`; der raylib-
  Desktop-Toggle ist dort deaktiviert, bleibt auf dem PC jedoch unverändert
- Einstellungsinhalt startet anhand der oberen Panelposition direkt unter dem Titel;
  der fehlerhafte vertikale Bezug zum linken Sicherheitsabstand ist entfernt
- Einstellungskarten bis `1300 px` breit; Abschnittstitel, Zeilentexte, Beschreibungen,
  Slider, Schalter und Controller-Auswahlbuttons vergrößert, Zwischenräume verdichtet
- Back-Button wird in den Einstellungen nach allen Karten erneut gezeichnet und
  bleibt dadurch vollständig sichtbar; Eingabe und Darstellung nutzen weiter dasselbe Rechteck
- Musik- und SFX-Karten berechnen Label-, Slider- und Prozentpositionen aus den
  tatsächlichen Textbreiten und halten jeweils `12 px` Abstand zum Prozentwert
- `RESET TO DEFAULT` als `250 × 44 px` große Schaltfläche oben rechts ergänzt;
  sichtbares Rechteck sowie Maus- und direkte Touchabfrage verwenden dieselbe Geometrie
- Reset stellt Master `100 %`, Musik `80 %`, SFX `100 %`, Audio an, Gegner an,
  Partikel mittel, Kamera `9,0`, Steuerung `SPLIT`, FPS aus und HUD `100 %` wieder her
  und speichert sofort; Fortschritts-, Shop- und Statistikdaten bleiben unberührt
- HUD, Inventar und Pause berücksichtigen die Android-Sicherheitsränder
- alle Musikdateien gegenüber dem vorherigen Build unverändert; dekodierte Messung
  des Menütracks: `48 kHz`, Stereo, Peak `0,982910`, keine Samples ab `0,999`
- ausschließlich der Menütrack erhält den Faktor `0,80`, wodurch sein maximaler
  rechnerischer Peak auf etwa `0,786` sinkt; Spielmusikpegel bleiben unverändert
- sämtliche Kaufpreise aus V1.5 wurden exakt halbiert: 30 bepreiste Skins,
  alle 23 Upgrade-Stufen und alle 6 Perks; Achievement-Auszahlungen blieben
  unverändert
- Android-Spielkamera auf `1,18×` vergrößert; Desktop-Zoom bleibt bei `1,00×`
- Hauptmenü, Untermenü-Titel und Shop-Tabs verwenden größere, klar erkennbare
  Symbole; Settings- und Future-Updates-Symbole wurden ebenfalls vergrößert
- Shop mit `150 px` großen Skin-Karten, größeren Skin-Vorschauen, Preisschriften,
  Kaufbuttons sowie `140 px` hohen Upgrade- und Perk-Karten
- Stats auf sechs große sichtbare Datenzeilen mit größeren Gruppenicons sowie
  größeren Beschriftungen und Werten umgestellt
- Achievements auf vier große sichtbare Karten mit größeren Texten, Sternen,
  Fortschrittsbalken und Belohnungsbuttons umgestellt
- Scrollgeschwindigkeit und Scrollleisten der vergrößerten Ansichten angepasst
- Kaufprüfung maximal ausgebauter Upgrades greift nicht mehr außerhalb des
  jeweiligen Preisarrays zu
- mathematische Joystick-Tests für Totzone, analoge Stärke, Vier-Richtungs-
  Zuordnung, maximalen Radius und nachziehenden Mittelpunkt bestanden
- dynamischer Touch-Stick entsteht an der tatsächlichen ersten Fingerposition im
  linken Bewegungsbereich und bleibt bis zum Loslassen an dieselbe Touch-ID gebunden
- Touch-Bewegung verwendet ausschließlich vollständige `TryMove`-Kachelschritte
  nach oben/unten/links/rechts und exakt den festen `pmd`-Takt der PC-Referenz
- `TryMove`-Block zeilengetreu mit `DungeonCrawlerV1_5/game_player.c` verglichen;
  keine Abweichung in Positions-, Kollisions- oder Kachelereignislogik
- halbtransparente Leerlaufdarstellung des Joysticks wird während des Spiels
  auch ohne aktive Berührung gezeichnet, ist größer und sitzt weiter links sowie höher
- neuer Standardmodus `SPLIT`: größerer Horizontaltaster links am Bildschirm steuert
  nur links/rechts; der größere Vertikaltaster liegt getrennt am rechten Bildschirmrand
  und steuert nur oben/unten; beide Richtungsflächen und Touch-Zonen wurden vergrößert
- `FLOAT` besitzt weiterhin vier sichtbare, diagonal getrennte Richtungssektoren
- festes Steuerkreuz bleibt als dritter Modus verfügbar
- `SETTINGS` → `MOBILE CONTROL` bietet `SPLIT`, `FLOAT` und `D-PAD`
- Steuerungsmodus und Formatversion werden als neuntes und zehntes Feld gespeichert;
  ältere Einstellungsdateien werden auf den neuen Standard `SPLIT` migriert
- zweite und weitere Touch-IDs bleiben für Pause und Inventaraktionen verfügbar
- native Android-`WindowInsets` für Systemleisten und Display-Cutouts werden aus
  physischen Displaypixeln in raylib-Koordinaten umgerechnet; ein zusätzlicher
  symmetrischer Mindestabstand hält `SKINS`, `UPGRADES`, `PERKS`, `STATS` und
  `ACHIEVEMENTS` innerhalb der sicheren Fläche
- die fünf betroffenen Scrollansichten verwenden auf Android eine ausschließlich
  vertikale Clip-Grenze; der native/logische Breitenunterschied kann Sterne,
  Kaufbuttons und Kartenränder dadurch nicht mehr horizontal abschneiden
- gewähltes Motiv „Inferno-Riss“ mit V1.6-Skull-Ember-Skin als transparenter
  `432 × 432`-Vordergrund exportiert; Eck-Alpha ist `0`
- Android-Adaptive-Icon mit getrennten `background`- und `foreground`-Ressourcen
  sowie separatem `roundIcon` im gepackten Manifest bestätigt
- zentrales Schädelmotiv und innerer Portalring liegen in der sicheren Icon-Zone;
  die äußeren Flammen dürfen von der jeweiligen Launcher-Maske beschnitten werden
- installierbare APK mit gültiger APK-Signatur v2
- Paket `de.projectdoubleup.dungeoncrawler`, Version `1.5.0`, `minSdk 26`, `targetSdk 36`
- 16-KB-Ausrichtungsprüfung der APK und nativen Bibliotheken erfolgreich
- alle 16 vorhandenen Original-Audiodateien im APK
- keine Netzwerk-, Kamera- oder externe Speicherberechtigung

## Noch auf realem Gerät zu prüfen

- tatsächliches Pixel-Launcher-Ergebnis für Kreis- und Themed-Icon-Darstellung
- sichtbare Darstellung und Lesbarkeit des Joysticks auf dem konkreten Telefon
- Haptik, Totzone, Reichweite und subjektive Input-Latenz des Analogsticks
- echtes Zwei-Finger-Spiel: Stick halten und gleichzeitig einen Item-Slot antippen
- alle Menüseiten per Touch/Wischgeste sowie Android-Zurück; insbesondere die
  rechten Ränder mit den vom Gerät gelieferten Systemleisten-/Cutout-Inset-Werten
- subjektive Lesbarkeit der neuen großen Shop-, Stats- und Achievement-Ansichten
  sowie der `1,18×`-Spielkamera auf dem konkreten Telefon
- eine komplette Runde einschließlich Tod, Neustart, Portal und nächster Runde
- Audiopegel/Latenz über den Telefonlautsprecher oder Kopfhörer

Beim Abschluss war weder ein Android-Gerät noch ein Emulator über ADB verbunden.
Der aktuelle Build ist daher vollständig gebaut, statisch geprüft und signiert,
aber nicht als visuelle oder haptische Realgeräteabnahme ausgewiesen.
