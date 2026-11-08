# Aquarium Computer

## Übersicht
Der ESP32-Aquarium Computer ist eine intelligente Steuerung für Aquarienbeleuchtung und -technik mit umfangreichen Automatisierungsmöglichkeiten.

## Funktionen

### Szenen-Verwaltung
- Verwaltung von **bis zu 15 verschiedenen Szenen**
- Jede Szene ist individuell konfigurierbar
- Automatische Umschaltung zwischen Szenen möglich

### Ausgänge und Steuerung
Der Controller verfügt über folgende Ausgänge:

**PWM-Ausgänge (3x)**
- 3 unabhängig steuerbare PWM-Kanäle
- PWM-Frequenz: 15 kHz, Auflösung: 12 Bit
- Steuersignale für externe MOSFET-Leistungsstufen

**Relais-Ausgänge (2x)**
- Separater Ausgang für das szenengesteuerte Relais
- Separater Ausgang für das Heizungsrelais
- Die GPIOs sind Steuersignale; geeignete Treiber bzw. Relaismodule sind erforderlich

## Beispielkonfiguration

Die folgende Konfiguration zeigt eine typische Anwendung:

| Kanal | Verwendung |
|-------|------------|
| **Channel 1** | Eheim classicLED plants (Pflanzenlicht) |
| **Channel 2** | Eheim classicLED daylight (Tageslicht) |
| **Channel 3** | Moonlight (Mondlicht) |
| **Relais** | CO2-Magnetventil |

## Technische Daten

- **Steuerung:** ESP32-basiert
- **Board:** WEMOS D1 MINI ESP32 (`esp32_d1_mini32`)
- **PWM-Kanäle:** 3x, 15 kHz, 12 Bit
- **Relais-Steuerausgänge:** 2x (Szenen und Heizung)
- **Temperatursensor:** DS18B20 am OneWire-Bus
- **Szenen:** bis zu 15 speicherbar

Die zulässige Spannung und Last hängen von den verwendeten MOSFET- und Relaismodulen ab. Die angegebenen GPIOs dürfen Verbraucher nicht direkt versorgen. Massgeblich sind die Datenblätter der Module und eine fachgerechte elektrische Auslegung.

## Konfiguration

Der Aquarium Computer wird über ein Webinterface konfiguriert. Die Einstellungen sind in mehrere Bereiche unterteilt:

### Erstinbetriebnahme

Beim Start ohne eingerichtete WLAN-Verbindung stellt das Gerät den Access Point **AquariumController** bereit. Das Standardpasswort lautet `123456789`; ändern Sie es in der Konfiguration. Rufen Sie nach dem Verbinden die Konfigurationsseite auf, tragen Sie WLAN-Zugangsdaten ein und speichern Sie die Einstellungen.

### System-Konfiguration

Diese Einstellungen betreffen die Netzwerkverbindung und grundlegende Systemparameter.

#### Thing Name
Der Gerätename im Netzwerk. Unter diesem Namen ist der Aquarium Computer im lokalen Netzwerk erreichbar.

#### AP Password
Passwort für den Access Point-Modus. Wenn das Gerät nicht mit einem WLAN verbunden ist, erstellt es einen eigenen Access Point.
- **Standard-Passwort:** `123456789`

#### WIFI SSID
Die SSID (Netzwerkname) des WLAN-Netzwerks, mit dem sich der Aquarium Computer verbinden soll.

#### WIFI Password
Das Passwort für das ausgewählte WLAN-Netzwerk.

### Zeitquelle

Konfiguration der Zeitsynchronisation für zeitgesteuerte Szenen.

#### Use NTP Server
Aktivieren Sie diese Option, um die Systemzeit automatisch über einen NTP-Server zu synchronisieren. Dies gewährleistet präzise Zeitsteuerung der Szenen.

#### NTP Server (FQDN or IP Address)
Adresse des NTP-Servers für die Zeitsynchronisation (z.B. `pool.ntp.org` oder `ntp.metas.ch`).

#### POSIX Timezone String
Zeitzoneneinstellung im POSIX-Format.
- **Standard:** `CET-1CEST,M3.5.0,M10.5.0/3` (Mitteleuropäische Zeit mit Sommer-/Winterzeit)

### Heizer-Konfiguration

Der Heizer verwendet einen DS18B20-Temperatursensor und einen eigenen Relais-Steuerausgang. In der Konfiguration lassen sich die Solltemperatur (17-30 °C, Standard 25 °C) und die Hysterese (0-5 °C) einstellen. Im Automatikmodus schaltet die Heizung unterhalb der Solltemperatur minus halber Hysterese ein und oberhalb der Solltemperatur plus halber Hysterese aus.

Bei einem erkannten Sensorfehler wird die Heizung im Automatikmodus aus Sicherheitsgründen abgeschaltet. Auch ein manuell erzwungenes Einschalten bleibt bei ungültigem Sensorwert standardmaessig gesperrt.

### Szenen-Konfiguration (Szene 1-15)

Jede der 15 Szenen kann individuell konfiguriert werden. Eine Szene definiert die Beleuchtungs- und Ausgangszustände zu einer bestimmten Uhrzeit.

#### Time (Uhrzeit)
Die Uhrzeit, ab der die Szene aktiv werden soll. Format: `HH:MM`

#### Channel 1 - 3
PWM-Wert in Prozent (0-100%) für jeden der drei Beleuchtungskanäle. Das System regelt die Werte automatisch weich (Fading) von der vorherigen zur nächsten Szene.

#### Relay (Relais)
Status des Relais-Ausgangs:
- **Ein:** Relais schaltet bei dieser Szene ein
- **Aus:** Relais schaltet bei dieser Szene aus

#### Jump (Sprung)
Aktivieren Sie diese Option, um das automatische Fading zu deaktivieren:
- **Aktiviert:** Die Werte werden sofort auf die eingestellten Werte gesetzt (harter Übergang)
- **Deaktiviert:** Die Werte werden weich von der vorherigen Szene überblendet (Standard)

#### Funktionsweise der Szenenverwaltung

**Automatische Interpolation:**
Das System interpoliert automatisch zwischen zwei aufeinanderfolgenden Szenen. Wenn Sie beispielsweise:
- Szene 1 um 08:00 mit 100% Helligkeit
- Szene 2 um 10:00 mit 50% Helligkeit

einstellen, wird die Helligkeit über die 2 Stunden hinweg linear von 100% auf 50% reduziert.

**Tagesübergänge (Day-Wrap-Around):**
Das System berücksichtigt auch Szenenübergänge über Mitternacht. Beispielsweise:
- Letzte Szene um 23:00 mit 10% Mondlicht
- Erste Szene um 06:00 mit 0% Mondlicht

Das System berechnet den Übergang korrekt über die Mitternacht hinweg.

**Ausgabe-Steuerungsmodi:**
Jeder der 4 szenengesteuerten Ausgänge (3 PWM-Kanäle + Szenen-Relais) kann in 3 Modi betrieben werden. Der Heizer wird separat gesteuert:
- **AUTO:** Das System folgt automatisch der aktuellen Szene (mit Interpolation zu nächster Szene)
- **ON:** Der Ausgang wird manuell eingeschaltet und ignoriert alle Szenenvorgaben
- **OFF:** Der Ausgang wird manuell ausgeschaltet und ignoriert alle Szenenvorgaben

Dies ermöglicht Überschreibungen der automatischen Szenensteuerung für Tests oder manuelle Steuerung.

**Sprungtransitionen:**
Ist das Jump-Flag einer Szene aktiviert, wird die Überblendung von der vorherigen Szene zu dieser Szene ausgelassen. Zum eingestellten Zeitpunkt werden die Werte der Szene übernommen. Das eignet sich für abrupte Lichtwechsel.

### Konfigurationsbeispiel
Um 06:00 leuchtet das Mondlicht mit 10%. Bis um 09:00 wird es auf 0% geregelt, gleichzeitig wird Channel 1 auf 30% geregelt. Von 09:00 bis 12:00 Uhr werden Channel 1 und 2 auf 100% geregelt. Ab 12:00 Uhr wird das Relais eingeschaltet und damit die CO2-Zufuhr aktiviert. Um 18:00 Uhr wird das Relais wieder deaktiviert, Channel 1 und 2 sind immer noch auf 100%. Diese werden nun bis 20:00 Uhr auf 30 beziehungsweise 0% geregelt. Ab 20:00 Uhr bis 22:00 Uhr wird Channel 1 auf 0% geregelt und das Mondlicht auf 10%. Dieses ist dann wieder bis 06:00 am nächsten Morgen auf 10%.

| Time | Channel 1 | Channel 2 | Channel 3 | Relay | Jump |
| :---: | :---: | :---: | :---: | :---: | :---: |
| 06:00 | 0 | 0 | 10 | off | off |
| 09:00 | 30 | 0 | 0 | off | off |
| 12:00 | 100 | 100 | 0 | on | off |
| 18:00 | 100 | 100 | 0 | off | off |
| 20:00 | 30 | 0 | 0 | off | off |
| 22:00 | 0 | 0 | 10 | off | off |

## Hardware

Für den Bau des Aquarium Computers wird folgende Hardware benötigt:

### Hauptkomponenten

**Steuereinheit**
- 1x WEMOS D1 MINI ESP32
  - Verwendet für zentrale Steuerung und WLAN-Verbindung

**PWM-Leistungsmodule**
- 3x geeignete MOSFET-Leistungsstufen für die Beleuchtung
  - Verwendet für die Beleuchtungssteuerung

**Relaismodule**
- Geeignete Relaismodule für den Szenenausgang (GPIO 16) und den Heizungsausgang (GPIO 25)

**Signalbelegung**

| Funktion | ESP32-GPIO |
|----------|------------|
| PWM Channel 1 | GPIO 33 |
| PWM Channel 2 | GPIO 26 |
| PWM Channel 3 | GPIO 14 |
| Szenen-Relais | GPIO 16 |
| Heizungsrelais | GPIO 25 |
| DS18B20 (OneWire) | GPIO 32 |

**Verkabelung**
- 4x M12 Wasserdichte Kabel-Steckverbinder (IP65)
  - Erhältlich mit 2, 3, 4 oder 5 Pins
  - Männliche und weibliche Varianten
  - Für sichere Verbindung zu LED-Strips und anderen Komponenten
  - Schutzart IP65 für Feuchträume geeignet

### Empfohlenes Zubehör
- Netzteil (je nach Beleuchtung 5-36V, ausreichend dimensioniert)
- Gehäuse zur Montage der Komponenten
- Kabel und Anschlussklemmen
- Montage-Material

### Hinweise zum Aufbau
- Achten Sie auf ausreichende Belüftung der MOSFET-Module bei hoher Last
- Verwenden Sie ausreichend dimensionierte Kabel (Querschnitt je nach Stromstärke)
- Die wasserdichten M12-Verbinder eignen sich besonders für den Einsatz in der Nähe des Aquariums
- Prüfen Sie die zulässigen Spannungen und Ströme anhand der Datenblätter der tatsächlich verwendeten Module
- Beachten Sie die Sicherheitshinweise beim Umgang mit Netzspannung und Feuchtigkeit

## Web-Interface

### Homepage

Die Homepage bietet einen Überblick über den aktuellen Status des Aquarium Computers und ermöglicht manuelle Steuerung der Ausgänge.

#### Informationsbereich
Die Homepage zeigt folgende Echtzeit-Informationen:

**Datum & Uhrzeit**
- Aktuelle Systemzeit, synchronisiert über NTP
- Wird automatisch jede Sekunde aktualisiert

**WiFi-Signalstärke (RSSI)**
- Empfangssignalstärke in dBm (Dezibel Milliwatt)
- Hilft zur Diagnose von WiFi-Verbindungsproblemen

**Aktuelle Werte**
- **Temperatur:** Aktuelle Wassertemperatur (wenn Sensor verbunden)
- **Heater Status:** Zeigt, ob der Heizer aktiv ist (ON/OFF)
- **Relay Status:** Anzeige des aktuellen Relais-Zustands

#### Manual Output Control

Dieser Bereich ermöglicht die manuelle Steuerung aller 4 Ausgänge unabhängig von den konfigurierten Szenen:

**Kanäle 1-3 (LED-Kanäle)**
Jeder LED-Kanal hat:
- **3 Schaltflächen:** On, Off, Auto
  - **On:** Setzt den Kanal auf volle Helligkeit (100%), aktiviert das Helligkeitseingabefeld
  - **Off:** Schaltet den Kanal aus (0%), deaktiviert das Helligkeitseingabefeld
  - **Auto:** Folgt automatisch der aktuellen Szenenkonfiguration mit Interpolation
- **Helligkeitseingabefeld:** Ermöglicht Eingabe von 0-100% (nur im On-Modus aktiviert)
- **Live-Aktualisierung:** Änderungen der Helligkeit werden sofort an den Controller übertragen

**Relais-Ausgang**
- 3 Schaltflächen: On, Off, Auto
- Ermöglicht manuelle Steuerung unabhängig von Szenen
- Ideal zum Testen oder für Notfallszenarien

**Heizer-Ausgang**
- 3 Schaltflächen: On, Off, Auto
- Manuelle Kontrolle der Heizfunktion

#### Funktionsweise der Webseite

**Echtzeit-Updates:**
- Daten werden alle 5 Sekunden vom Controller abgerufen
- Temperatur, RSSI und Helligkeitswerte werden automatisch aktualisiert
- Der Link **Live Events** öffnet den Ereignis- und Diagnose-Stream

#### Web-Befehle über Live Events

Die Befehle werden im Live-Events-Fenster eingegeben. Gross- und Kleinschreibung wird ignoriert.

- `help` - zeigt die verfügbaren Befehle
- `status` - zeigt Uhrzeit, WLAN-Signalstärke, Kanalwerte, Relais- und Heizerstatus sowie Temperatur und Heizerparameter
- `memory` - zeigt Heap- und Flash-Speicherinformationen
- `verbose` - schaltet die periodischen Temperaturmeldungen im Ereignis-Stream ein oder aus
- `forceheat` - schaltet die Sensorfehler-Übersteuerung für den manuellen Heizer-Modus ein oder aus

**Achtung:** `forceheat` erlaubt das Heizen im manuellen Modus trotz fehlendem oder fehlerhaftem Temperatursensor. In diesem Zustand besteht kein Überhitzungsschutz. Die Übersteuerung wird beim Neustart zurückgesetzt.

**Benutzerinteraktion:**
- Klick auf On/Off/Auto-Buttons sendet sofort einen POST-Request an den Controller
- Änderung der Helligkeitswerte wird registriert und übertragen
- Seite wird nach erfolgreicher Änderung neu geladen

**Responsive Design:**
- Flexibles Layout mit Tabs/Feldern für gute Lesbarkeit
- Farbcodierung der Buttons für klare Status-Anzeige:
  - **Grün:** On-Status
  - **Rot:** Off-Status
  - **Blau:** Auto-Status

#### Navigation

Am unteren Ende der Homepage finden Sie:
- **Configuration Page Link:** Öffnet die Konfigurationsseite für erweiterte Einstellungen
- **Reboot Button:** Startet den Controller neu
- **Home Button:** Kehrt zur Homepage zurück
- **Reset to factory defaults:** Setzt alle Einstellungen auf Standardwerte zurück

## Software

### Entwicklungsumgebung
Das Projekt ist für Visual Studio mit Visual Micro eingerichtet und verwendet das ESP32-Arduino-Framework. Als Zielboard ist der WEMOS D1 MINI ESP32 (`esp32_d1_mini32`) konfiguriert. Eine PlatformIO-Konfiguration ist im Repository nicht enthalten.

### Erforderliche Bibliotheken

Für den Build werden das ESP32-Boardpaket und die folgenden Bibliotheken benötigt:

**Im ESP32-Arduino-Framework enthalten**
- `WiFi`, `DNSServer`, `Preferences` und `ArduinoOTA`

**Zusätzliche Bibliotheken**
- `IotWebConfAsync`, `IotWebConfAsyncUpdateServer` und `IotWebRoot`
- `ESP Async WebServer` und `AsyncTCP`
- `ESP32PWM` (aus ESP32Servo)
- `OneWire` und `DallasTemperature` für den DS18B20
- `NTPClient` und `EventLogger`

`neotimer` ist im Repository enthalten. Die übrigen oben genannten Zusatzbibliotheken sowie die in der Visual-Micro-Projektdatei eingebundenen Geschwisterprojekte sind separate Abhängigkeiten.

### Projekt-spezifische Module
Die folgenden Module sind Teil des Projekts und bereits enthalten:
- `version.h` - Versionsverwaltung
- `NTPSettings.h` - NTP-Konfiguration
- `Scene.h` / `Scene.cpp` - Szenen-Verwaltung
- `handlingWeb.h` / `handlingWeb.cpp` - Web-Interface und Konfiguration
- `heater.h` / `heater.cpp` - Temperaturmessung und Heizungsregelung
- `html.h` - HTML-Templates für Web-Interface
- `favicon.h` - Favicon für Web-Interface
- `board.txt` - Board-Konfiguration

### Build und Upload

Öffnen Sie das Visual-Micro-Projekt, installieren Sie das ESP32-Boardpaket und die genannten externen Abhängigkeiten und wählen Sie das konfigurierte Board **WEMOS D1 MINI ESP32**. `src/board.txt` enthält Build-Hooks für lokale PowerShell-Skripte zur Versionsverwaltung und zum Kopieren der Binärdatei; die dort konfigurierten Pfade müssen in der jeweiligen Entwicklungsumgebung verfügbar sein.

### Upload-Methoden
- USB-Kabel (erste Inbetriebnahme)
- OTA (Over-the-Air) für Updates über WLAN