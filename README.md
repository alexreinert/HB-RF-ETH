# HB-RF-ETH Firmware

### Unterstützung [![Spenden](https://img.shields.io/badge/donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=WUC7QU84EU7DA)
Meine Entwicklungen im Homematic Umfeld sind sehr kostenintensiv, z.B. werden viele verschiedene Testgeräte oder auch diverse Prototypen von Platinen benötigt. Allerdings erhält meine Projekt keine Unterstützung durch kommerzielle Anbieter. Ich freue mich daher durch eine Unterstützung per [Github Sponsors](https://github.com/sponsors/alexreinert), via [PayPal](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=WUC7QU84EU7DA) oder durch eine Aufmerksamkeit auf meinem [Amazon Wunschzettel](https://www.amazon.de/gp/registry/wishlist/3NNUQIQO20AAP/ref=nav_wishlist_lists_1).

### Worum es geht
Dieses Repository enhält die Firmware für die HB-RF-ETH Platine, welches es ermöglicht, ein Homematic Funkmodul HM-MOD-RPI-PCB oder RPI-RF-MOD per Netzwerk an eine debmatic oder piVCCU3 Installation anzubinden.

Hierbei gilt, dass bei einer debmatic oder piVCCU3 immer nur ein Funkmodul angebunden werden kann, egal ob diese direkt per GPIO Leiste, USB mittels HB-RF-USB(-2) Platine oder per HB-RF-ETH Platine erfolgt.

### Was kann die Firmware
* Bereitstellung des Funkmoduls RPI-RF-MOD oder HM-MOD-RPI-PCB per UDP als raw-uart Gerät inkl. Ansteuerung der LEDs des RPI-RF-MODs
* (S)NTP Server für die Verteilung der Zeit im lokalen Netzwerk
* Unterstützung der RTC des RPI-RF-MODs oder eines [DS3231 Aufsteckmoduls](https://www.amazon.de/ANGEEK-DS3231-Precision-Arduino-Raspberry/dp/B07WJSQ6M2)
* Verschiedene mögliche Zeitquellen
  * (S)NTP Client
  * DCF77 Empfänger (aka Funkuhr) mittels [optionalem Moduls](https://de.elv.com/elv-gehaeuse-fuer-externe-dcf-antenne-dcf-et1-komplettbausatz-ohne-dcf-modul-142883):
    * Konnektor J5
    * Pin 1: VCC
    * Pin 2: DCF Signal
    * Pin 3: Gnd
  * GPS Empfänger mittels [optionalem Moduls](https://www.amazon.de/AZDelivery-NEO-6M-GPS-baugleich-u-blox/dp/B01N38EMBF):
    * Konnektor J5
    * Pin 1: VCC
    * Pin 2: TX
    * Pin 3: Gnd
* MDNS Server um Platine im Netzwerk bekannt zu machen
* Netzwerkeinsellungen per DHCP oder statisch konfigurierbar
* WebUI zur Konfiguration
  * Intialpasswort: admin
* Firmware Update per Webinterface
* Regelmäßige Prüfung auf Firmwareupdates
* Werksreset per Taster

### Werksreset
Die Firmware kann per Taster auf Werkseinstellungen zurückgesetzt werden:
1. Platine vom Strom trennen
2. Taster drücken und gedrückt halten
3. Stromversorgung wiederherstellen
4. Nach ca. 4 Sekunden fängt die rote Status LED schnell zu blinken an und die grüne Power LED hört auf zu leuchten
5. Taster kurz loslassen und wieder drücken und gedrückt halten
6. Nach ca. 4 Sekunden leuchten die grüne Power LED und die rote Status LED für eine Sekunde
7. Danach ist der Werkreset abgeschlossen und es folgt der normale Bootvorgang

### Blinkcodes der LEDs
#### RPI-RF-MOD
Siehe Hilfe zum RPI-RF-MOD

#### Grüne Power LED und rote Status LED
* Blinken abwechselnd mit grüner Power LED: System bootet
* Schnelles Blinken der roten Status LED, grüne Power LED leuchtet nicht: Siehe Werksreset
* Schnelles Blinken der roten Status LED, grüne Power LED leuchtet dauerhaft: Firmware Update wird eingespielt
* Langsames Blinken der roten Status LED, grüne Power LED leuchtet dauerhaft: Es ist ein Firmware Update verfügbar
* Dauerhaftes Leuchten der grünen Power LED: Sytem ist gestartet

### Firmware Updates
Firmware Updates sind fertig kompiliert und Releases zu finden und können per Webinterface eingespielt werden.

### Einbindung in piVCCU3 und debmatic
/TBD/

### Roadmap
Folgende Punkte sind angedacht für zukünftige Releases. Die Sortierung ist als zufällig anzusehen und es ist nicht garantiert, dass alle Punkte auch umgesetzt werden.

* WebUI: Aktuellen Status anzeigen
* WebUI: Besseres parsing multipart/form-data bei OTA Update
* Transportverschlüsselung raw-uart
* Detailerkennung des aufgesteckten Funkmoduls
* LED Fading
* SNMP
* CheckMK Agent
* LAN GW Modus
* AskSin Analyzer Light

## Lizenz
Die Firmware steht unter Apache 2.0 Lizenz.
