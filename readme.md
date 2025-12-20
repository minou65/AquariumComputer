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
- Hochleistungs-PWM-Ausgänge
- Betriebsspannung: 5-36V
- Dauerlast: 15A pro Kanal
- Maximallast: 30A (kurzzeitig)
- Leistung: bis zu 400W

**Relais-Ausgang (1x)**
- 1 schaltbarer Relais-Ausgang
- Schaltlast: bis zu 10A
- Ideal für Magnetventile und andere Verbraucher

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
- **PWM-Kanäle:** 3x, 5-36V, max. 15A/400W
- **Relais:** 1x, max. 10A Schaltlast
- **Szenen:** bis zu 15 speicherbar

## Konfiguration

Der Aquarium Computer wird über ein Webinterface konfiguriert. Die Einstellungen sind in mehrere Bereiche unterteilt:

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

### Konfigurationsbeispiel
Um 06:00 leuchtet das Mondlicht mit 10%. Bis um 09:00 wird es auf 0% geregelt, gleichzeit wird Channel 1 auf 30% geregelt. Von 09:00 bis 12:00 Uhr werden Channel 1 und 2 auf 100% geregelt. Ab 12:00 Uhr wird das Relais eingeschaltet und damit die CO2 zufuhr aktiviert. Um 18:00 Uhr wird das Relais wieder deaktiviert, Channel 1 und 2 sind immer noch auf 100%. Diese werden nun bis 20:00 Uhr auf 30 resp. 0% geregelt. Ab 20:00 Uhr bis 22:00 wird Channel 1 auf 0% regelt und das Mondlicht auf 10%. Dieses ist dann wieder bis 06:00 am nächsten Morgen auf 10%

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
- 1x ESP32 WiFi/Bluetooth Board mit Relais-Ausgang
  - Spezifikation: DC 7-60V, 1-Kanal Relais
  - Verwendet für zentrale Steuerung und Relais-Schaltung

**PWM-Leistungsmodule**
- 3x DC 5V-36V Hochleistungs-MOSFET-Trigger
  - Dauerlast: 15A pro Modul
  - Spitzenlast: max. 30A
  - Leistung: bis zu 400W pro Kanal
  - Verwendet für die Beleuchtungssteuerung

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
- Beachten Sie die Sicherheitshinweise beim Umgang mit Netzspannung

## Software

### Entwicklungsumgebung
Die Software ist für die Arduino IDE oder PlatformIO entwickelt und basiert auf dem ESP32-Framework.

### Erforderliche Bibliotheken

Folgende Bibliotheken werden benötigt, um die Software zu kompilieren:

**Grundlegende ESP32-Bibliotheken**
- `Arduino.h` - Arduino Core für ESP32
- `WiFi.h` - WiFi-Funktionalität für ESP32

**Netzwerk und Web-Interface**
- `IotWebConfAsync` - Asynchrones Web-Konfigurationsframework
- `IotWebConfAsyncUpdateServer` - OTA-Update-Server
- `IotWebRoot` - Web-Root-Handling

**Zeit und Timer**
- NTP-Client (für Zeitsynchronisation)
- `neotimer` - Timer-Bibliothek (im Projekt enthalten)

### Projekt-spezifische Module
Die folgenden Module sind Teil des Projekts und bereits enthalten:
- `version.h` - Versionsverwaltung
- `NTPSettings.h` - NTP-Konfiguration
- `ntp.h` - NTP-Client-Implementierung
- `Scene.h` / `Scene.cpp` - Szenen-Verwaltung
- `handlingWeb.h` / `handlingWeb.cpp` - Web-Interface und Konfiguration
- `html.h` - HTML-Templates für Web-Interface
- `favicon.h` - Favicon für Web-Interface
- `board.txt` - Board-Konfiguration

### Installation der Bibliotheken

**Arduino IDE:**
1. Öffnen Sie den Library Manager (Sketch → Include Library → Manage Libraries)
2. Suchen und installieren Sie die erforderlichen Bibliotheken
3. ESP32 Board Support über Board Manager installieren

**PlatformIO:**
Die Abhängigkeiten können in der `platformio.ini` definiert werden. Die meisten Bibliotheken werden automatisch heruntergeladen.

### Kompilierung
1. Öffnen Sie das Projekt in Ihrer IDE
2. Wählen Sie das richtige ESP32-Board aus
3. Kompilieren und hochladen Sie die Software auf den ESP32

### Upload-Methoden
- USB-Kabel (erste Inbetriebnahme)
- OTA (Over-the-Air) für Updates über WLAN