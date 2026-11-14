# HB-RF-ETH Firmware

> **Hinweis:** Dieses Repository ist ein Fork von (https://github.com/alexreinert/HB-RF-ETH)), ursprünglich entwickelt von Alexander Reinert. Die Urheberschaft des ursprünglichen Projekts liegt vollständig beim Originalautor.
> 
> **Zweck dieses Forks:** Erweiterung der Firmware um eine API und MQTT zur Überwachung des Verbindungszustandes zwischen OpenCCU und der HB-RF-ETH.
> 

### Unterstützung [<img src="https://ko-fi.com/img/githubbutton_sm.svg" height="20" alt="Support me on Ko-fi">](https://ko-fi.com/alexreinert) [<img src="https://img.shields.io/badge/donate-PayPal-green.svg" height="20" alt="Donate via Paypal">](https://www.paypal.com/donate/?cmd=_s-xclick&hosted_button_id=4PW43VJ2DZ7R2)
Meine (Alexander Reinert) Entwicklungen im Homematic Umfeld sind sehr kostenintensiv, z.B. werden viele verschiedene Testgeräte oder auch diverse Prototypen von Platinen benötigt. Allerdings erhält meine Projekt keine Unterstützung durch kommerzielle Anbieter. Ich freue mich daher durch eine Unterstützung mit einer Spende via [Ko-fi](https://ko-fi.com/alexreinert), [PayPal](https://www.paypal.com/donate/?cmd=_s-xclick&hosted_button_id=4PW43VJ2DZ7R2) oder durch eine Aufmerksamkeit auf meinem [Amazon Wunschzettel](https://www.amazon.de/gp/registry/wishlist/3NNUQIQO20AAP/ref=nav_wishlist_lists_1).

### Worum es geht
Dieses Repository enhält die Firmware für die HB-RF-ETH Platine, welches es ermöglicht, ein Homematic Funkmodul HM-MOD-RPI-PCB oder RPI-RF-MOD per Netzwerk an eine debmatic oder piVCCU3 Installation anzubinden.

Hierbei gilt, dass bei einer debmatic oder piVCCU3 Installation immer nur ein Funkmodul angebunden werden kann, egal ob die Anbindung direkt per GPIO Leiste, USB mittels HB-RF-USB(-2) Platine oder per HB-RF-ETH Platine erfolgt.

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
* Erkennung des Funkmoduls und Ausgabe von Typ, Seriennummer, Funkadresse und SGTIN in der WebUI
* Regelmäßige Prüfung auf Firmwareupdates
* Werksreset per Taster
* REST-API zum Auslesen des System- und Funkmodulstatus (siehe unten)
* MQTT Anbindung inkl. optionaler Home Assistant Discovery (siehe unten)

### REST-API
Der Status kann ohne Anmeldung per `GET http://<hb-rf-eth>/api/v1/status` als JSON abgerufen werden:

```json
{
  "device": {
    "serial": "8CAAB5AA186B",
    "hostname": "HB-RF-ETH-AA186B",
    "boardType": "REV 1.10 PUB",
    "firmwareVersion": "1.3.1",
    "latestFirmwareVersion": "1.3.1",
    "uptime": 86400,
    "cpuUsage": 1.95,
    "memoryUsage": 41.71,
    "ipAddress": "192.168.20.10"
  },
  "ccu": {
    "connected": true,
    "address": "192.168.20.2"
  },
  "radioModule": {
    "detected": true,
    "type": "HM-MOD-RPI-PCB",
    "serial": "OEQ0607369",
    "bidCosRadioMAC": "0x59D9A9",
    "hmIPRadioMAC": "0xBE8379",
    "sgtin": "3014F711A061A7D70992AC89",
    "firmwareVersion": "1.4.1"
  }
}
```

* `uptime` in Sekunden, `cpuUsage` und `memoryUsage` in Prozent
* `ccu.connected` ist `true`, solange eine CCU (debmatic / piVCCU3) per raw-uart verbunden ist, `ccu.address` ist dann ihre IP Adresse

### MQTT
MQTT wird in der WebUI unter Einstellungen aktiviert (Broker, Port, optional Benutzer/Passwort, Topic Präfix, Sendeintervall, Home Assistant Discovery). Änderungen werden nach einem Neustart aktiv.

| Topic | Inhalt |
| --- | --- |
| `<präfix>/status` | `online` bzw. `offline` (retained, `offline` als Last Will) |
| `<präfix>/state` | JSON, identisch zu `/api/v1/status` (retained) |

Der Präfix ist standardmäßig `hb-rf-eth/<Seriennummer>`. Der Status wird im eingestellten Intervall (Standard 60 Sekunden) und zusätzlich sofort bei einer Verbindung oder Trennung der CCU gesendet.

Ist die Home Assistant Discovery aktiv, werden unter `homeassistant/...` Konfigurationen gesendet, sodass Home Assistant das HB-RF-ETH automatisch als Gerät mit Sensoren (u.a. CCU verbunden, Funkmodultyp, Seriennummer, Funkadressen, SGTIN, CPU/Speicher, Uptime) anlegt.

### Bekannte Einschränkungen
* Nach einem Neustart der Platine (z.B. bei Stromausfall) findet kein automatischer Reconnect statt, in diesem Fall muss die CCU Software daher neu gestartet werden.
* Die Stromversorgung mittels des Funkmoduls RPI-RF-MOD darf nur erfolgen, wenn keine andere Stromversorgung (USB oder PoE) angeschlossen ist.

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
Firmware Updates sind fertig kompiliert und Releases zu finden und können per Webinterface eingespielt werden. Zum Übernehmen der Firmware muss die Platine neu gestartet werden (mittel Power-On Reset).

### Einbindung in piVCCU3 und debmatic
Die Unterstützung für die Platine HB-RF-ETH ist in piVCCU3 ab Version 3.51.6-41 und in debmatic ab Version 3.51.6-46 eingebaut. Die Installation der Platine erfolgt über das Paket "hb-rf-eth". Weiteres Details findet man in der Installationsanleitung von piVCCU3 bzw. debmatic.

### Wo bekomme ich die Hardware her?
Die Platine ist veröffentlicht auf [Github](https://github.com/alexreinert/PCB/?tab=readme-ov-file#hb-rf-eth) und kann selbst gefertigt werden.
Alternativ kann diese auch als Bausatz mit vollständig vorbestückten SMD Bauteilen bei [viopcb](https://www.viopcb.de/Bausatz-HB-RF-ETH-mit-PoE) gekauft werden.

### Roadmap
Folgende Punkte sind angedacht für zukünftige Releases. Die Sortierung ist als zufällig anzusehen und es ist nicht garantiert, dass alle Punkte auch umgesetzt werden.

* Transportverschlüsselung raw-uart
* LED Fading
* SNMP
* CheckMK Agent
* LAN GW Modus
* AskSin Analyzer Light

### Lizenz
Die Firmware steht unter Creative Commons Attribution-NonCommercial-ShareAlike 4.0 Lizenz.
