# Dungeon Crawler v1.7 für das Web

Dungeon Crawler: The Secret Chambers ist ein in C mit raylib entwickeltes Spiel. Der Spieler erkundet zufällig erzeugte Räume, weicht Gegnern aus und sammelt Münzen, Schätze und Items. Dieses Archiv enthält die Browserfassung mit deutscher und englischer Website-Oberfläche. Beide Sprachfassungen verwenden dasselbe Spiel.

[Deutsch spielen](https://smstark-projekt.de/playdungeoncrawler/v1.7/) | [Play in English](https://smstark-projekt.de/playdungeoncrawler/v1.7/en/) | [Projektseite](https://smstark-projekt.de/projekte/dungeoncrawler)

## Start

Erforderlich ist ein aktueller Browser mit WebAssembly- und WebGL-Unterstützung. Entpacken Sie das Archiv vollständig. Die Dateien im Ordner `web` müssen über einen HTTP- oder HTTPS-Server bereitgestellt werden.

Für einen lokalen Start mit installiertem Python 3:

1. Öffnen Sie ein Terminal im entpackten Ordner `web`.
2. Starten Sie den Server mit `python -m http.server 8000`.
3. Öffnen Sie `http://localhost:8000/` im Browser. Die englische Seite liegt unter `http://localhost:8000/en/`.

Für die Veröffentlichung auf einem eigenen Webserver laden Sie den vollständigen Inhalt von `web` hoch. Die Unterordner `en` und `assets` müssen erhalten bleiben. Der DE/EN-Schalter wechselt die Website-Sprache. Das Logo und die Projektlinks führen zu den entsprechenden Seiten auf `smstark-projekt.de`.

## Steuerung

| Eingabe | Funktion |
| --- | --- |
| W, A, S, D oder Pfeiltasten | Bewegen |
| Maus oder Tastatur | Menüs bedienen |
| Esc | Pause oder zurück |
| Schaltfläche „Vollbild“ | Vollbild einschalten |

Ein Klick in die Spielfläche aktiviert die Tonausgabe. Die Download- und Projektlinks oberhalb des Spiels bleiben beim Öffnen von Untermenüs und während des Spielens sichtbar. Die Projektseite öffnet sich in einem neuen Tab.

## Startausstattung

Die Webversion richtet beim ersten Start folgende Ausstattung ein:

| Bereich | Startwert |
| --- | --- |
| Speed Boost, Shield, Ghostwalk, Bomb Radius und Gold Touch | Upgrade-Stufe 3 |
| Sword Reach | Upgrade-Stufe 2 |
| Inventar | Drei Item-Slots, einschließlich der freigeschalteten Slots 2 und 3 |
| Coin-Guthaben | 250 Coins |

Bestehende Browser-Spielstände erhalten diese Ausstattung einmalig, soweit die jeweiligen Werte darunter liegen. Höhere Upgrades, freigeschaltete Perks und ein höheres Guthaben bleiben erhalten. Weitere Käufe und ausgegebene Coins werden gespeichert; ein Neuladen vergibt das Startguthaben nicht erneut.

## Dateien und Spielstände

`web/index.html` ist die deutsche Startseite, `web/en/index.html` die englische. `game.js`, `game.wasm` und `game.data` enthalten das Spiel und seine Ressourcen. Der Ordner `assets` enthält die mitgelieferten Dateien für Logo, Sprachschalter und Schriften.

Spielstände liegen lokal im Browserspeicher. Beide Sprachfassungen teilen den Spielstand, wenn sie über dieselbe Adresse bereitgestellt werden. Eine andere Domain oder ein anderer Port verwendet einen eigenen Browserspeicher. Durch das Löschen der Browserdaten gehen auch die dort gespeicherten Spielstände verloren. Das Archiv enthält keine persönlichen Spielstände.

Die Schriften Syne und DM Sans stehen unter der SIL Open Font License. Die zugehörigen Lizenztexte liegen unter `web/assets/fonts/LICENSE-Syne.txt` und `web/assets/fonts/LICENSE-DM-Sans.txt`.

[PC-Version](https://smstark-projekt.de/data/DungeonCrawlerV1_7.zip) | [GitHub](https://github.com/simons-0427/dungeon-crawler)
