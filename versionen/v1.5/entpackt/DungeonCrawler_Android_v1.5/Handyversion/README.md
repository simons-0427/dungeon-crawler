# Dungeon Crawler für Android

Dies ist die Android-Portierung von `DungeonCrawlerV1_5`. Der bestehende
C99/raylib-Spielkern bleibt die Quelle für Spielregeln, Gegner, Karten,
Gegenstände, Progression, Menüs, Grafik und Audio. Android Studio/Gradle/CMake
übernehmen nur NDK-Build, APK-Paketierung und die mobile Plattformschicht.

## APK installieren

Die direkt installierbare APK liegt im Projektstamm:

`DungeonCrawler-Android-v1.5.0.apk`

Sie benötigt Android 8.0 oder neuer und startet im Querformat. Für eine manuelle
Installation muss Android die Installation aus der verwendeten Datei-App bzw.
dem Browser erlauben.

Die APK ist mit dem lokalen Android-Debugzertifikat signiert. Sie ist für
Sideloading und Gerätetests gedacht, nicht als endgültige Play-Store-Signatur.

## Steuerung

- Menüs und Schaltflächen: antippen
- Bewegung: Im Standardmodus links am Bildschirm links/rechts und rechts am
  Bildschirm oben/unten steuern
- Steuerungsart: unter `SETTINGS` → `MOBILE CONTROL` zwischen `SPLIT`, `FLOAT`
  und festem `D-PAD` wechseln
- Gegenstände: Inventar-Slot oben antippen; funktioniert gleichzeitig mit dem Stick
- Pause: Pause-Symbol oben rechts oder Android-Zurück
- Scrollbare Menüs: vertikal wischen
- Tod/Rundenende: eingeblendete Touch-Schaltflächen verwenden

Standardmäßig ist `SPLIT` aktiv: Am linken Bildschirmrand sitzt ein großer
horizontaler Taster ausschließlich für links/rechts. Der ebenfalls große vertikale
Taster für oben/unten sitzt getrennt davon am rechten Bildschirmrand. Beide
haben großzügige Touch-Flächen und bleiben ohne Berührung halbtransparent sichtbar.
`FLOAT` verwendet weiterhin den
größeren, in vier Richtungssektoren unterteilten und mitwandernden Controller. Im
Modus `D-PAD` bleibt stattdessen ein klassisches Steuerkreuz fest an seiner Position.

Auf Android werden Systemleisten und Display-Aussparungen über die tatsächlichen
`WindowInsets` des Geräts berücksichtigt. Menüs und Touch-Controller bleiben dadurch
auch im erzwungenen Edge-to-edge-Modus moderner Android-Versionen in der sicheren Zone.
Die scrollbaren Menüs beschneiden nur vertikal; ihre rechte Seite wird nicht mehr
durch die abweichende native Android-Oberflächenbreite abgeschnitten.

Alle drei Modi bewegen die Figur ausschließlich um vollständige Kacheln. Nach
Verlassen der kleinen Totzone gilt exakt der feste Original-Bewegungstakt des
PC-Spiels; Auslenkung und Fingerentfernung verändern das Tempo nicht. Die Auswahl
wird in `dungeons_settings.txt` gespeichert. Tastatursteuerung und die
ursprünglichen Spielereignisse bleiben erhalten.

Das Launcher-Icon verwendet auf Android 8.0 und neuer getrennte Adaptive-Icon-
Ebenen für Vordergrund und Hintergrund. Das zentrale Inferno-Riss-Motiv liegt in
der sicheren Zone; Android darf die äußeren Flammen passend zur Geräteform als
Kreis, Squircle oder eine andere unterstützte Maske beschneiden.

## Handyoptimierte Darstellung und Preise

Auf Android ist die Spielkamera auf `1,18×` gestellt. Dadurch sind Spielfigur,
Kacheln, Gegner und Gegenstände während des Spielens näher und besser erkennbar;
die Desktop-Kamera bleibt unverändert.

Die Navigation verwendet deutlich größere Symbole. Shop-Tabs, Skin-Vorschauen,
Kaufbuttons, Upgrade- und Perk-Karten wurden vergrößert. `STATS` zeigt weniger,
dafür wesentlich größere Einträge gleichzeitig; `ACHIEVEMENTS` verwendet große
Karten, Sterne, Fortschrittsanzeigen und Belohnungsbuttons. Alle langen Ansichten
bleiben per Touch vertikal scrollbar.

Der sichtbare Back-Button und seine Touch-Fläche verwenden dieselbe, an die
Android-Sicherheitsränder angepasste Position und Größe. Ein Fingertipp wird
zusätzlich direkt als Touch-Eingabe erkannt und funktioniert damit auf allen
Unterseiten über die gesamte sichtbare Buttonfläche.

Das Spiel-HUD wurde für kleine Displays neu skaliert: Die obere Leiste ist
kompakte `88 px` hoch, ihre nochmals vergrößerten Texte und Symbole beginnen direkt
unterhalb der sicheren oberen Bildschirmkante und die bis zu fünf
Inventarplätze besitzen jeweils eine sichtbare und antippbare Fläche von
`86 × 86 px`. Der Pause-Button misst `72 × 72 px`; das Pausenfenster und seine
beiden `72 px` hohen Schaltflächen passen sich der Bildschirmgröße an. Die
Desktop-Hinweise `[E] Enemies` und `[F11] Fullscreen` werden im UI nicht mehr
angezeigt; die zugehörigen Desktop-Tastenfunktionen bleiben erhalten.

Die Kill-Anzeige verwendet ein flaches 2D-Symbol passend zur Bestwertzeile.
Rundenabschluss-Texte wie `TREASURE FOUND!`, `PORTAL USED!` und `YOU DIED` sowie
die zugehörigen Ergebnis- und Buttontexte werden auf dem Handy größer dargestellt;
`Coins / Kills` nutzt `36 px`, `NEXT ROUND` `28 px`. Der gesamte
`NEXT ROUND`-Button sitzt mit zusätzlichem Abstand unterhalb der Ergebniszeile.

Android startet über eine eigene raylib-kompatible `NativeActivity` dauerhaft im
Immersive-Fullscreen-Modus. Status- und Navigationsleiste sind ausgeblendet und
werden nach Rückkehr zur App oder einem Fokuswechsel automatisch wieder verborgen.
Ab Android 11 wird der native `WindowInsetsController` verwendet; Android 8 bis 10
nutzt `IMMERSIVE_STICKY`. Eine Randgeste kann die Systemleisten weiterhin kurzzeitig
einblenden, Android blendet sie anschließend automatisch aus. Die Fullscreen-Option
ist auf Android fest auf `ON` gesetzt und kann die Spielfläche nicht verkleinern.

Das Einstellungsmenü beginnt direkt unter seinem Titel. Der frühere vertikale
Versatz durch den linken Sicherheitsabstand wurde entfernt. Karten, Überschriften,
Beschreibungen, Regler, Schalter und die drei Controller-Auswahlbuttons sind größer
und breiter, während Zeilen- und Abschnittsabstände kompakt bleiben. Der Back-Button
wird nach sämtlichen Einstellungskarten erneut gezeichnet und kann nicht übermalt
werden; seine zentrale Touchfläche bleibt unverändert.

Die Musik- und SFX-Zeile berechnet die Positionen von Beschriftung, Regler und
Prozentwert anhand ihrer tatsächlichen Textbreite, sodass die Balken keinen Text
überdecken. `RESET TO DEFAULT` oben rechts setzt nur die Einstellungen auf ihre
Ausgangswerte zurück; Spielfortschritt, Skins, Upgrades, Perks, Statistiken und
Achievements bleiben unberührt.

Der Menütrack `safe_haven.mp3` bleibt als Originaldatei erhalten, erhält beim
Abspielen auf dem Handy aber `20 %` Headroom. Sein gemessener Spitzenpegel sinkt
dadurch bei maximaler Einstellung rechnerisch von `0,9829` auf etwa `0,786`, um
Bröckeln bzw. Lautsprecherverzerrung im Home-Menü zu vermeiden. Die Lautstärke der
Spielmusik wird dadurch nicht verändert.

Sämtliche Kaufpreise des V1.5-Shops wurden halbiert: alle Skins, alle fünf
Upgrade-Reihen und alle sechs Perks. Achievement-Belohnungen bleiben unverändert,
da sie Auszahlungen und keine Preise sind.

## Lokal bauen

Voraussetzungen dieser Projektkonfiguration:

- JDK 17 oder neuer
- Android SDK Platform 36.1
- Android NDK `28.2.13676358`
- CMake `3.22.1`

Debug-APK und Lint:

```powershell
.\gradlew.bat --offline --no-daemon lintDebug assembleDebug
```

Gradle erzeugt die APK zusätzlich unter
`app/build/outputs/apk/debug/app-debug.apk`.

## Projektstruktur

- `app/src/main/cpp/game/`: portierter Dungeon-Crawler-C-Code
- `app/src/main/cpp/mobile_input.*`: Analogstick, Multi-Touch und Wischgesten
- `app/src/main/java/.../FullscreenNativeActivity.java`: Android Immersive Fullscreen
- `app/src/main/res/mipmap-anydpi-v26/`: moderne Adaptive-Icon-Definitionen
- `app/src/main/assets/assets/`: Originalmusik und Original-SFX
- `third_party/raylib/`: eingebettetes raylib 6.0 samt Lizenz
- `TEST_REPORT.md`: durchgeführte Prüfungen und verbleibende Geräteabnahme

Hinweis: `enemy_kill.wav/.mp3` fehlt bereits im gelieferten V1.5-Assetbestand.
Das Spiel behandelt dies weiterhin als optionalen, stillen Effekt und läuft
normal weiter.
