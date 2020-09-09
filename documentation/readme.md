
h1. Paket Struktur unseres Protokolls

h2. Grundsätzlicher Aufbau des eigenen Binär-Protokolls

`|StartByte|CMD (low)|[CMD high]|[Destination]|Request ID|N Bytes (low)|[N Bytes (high)]|[Checksum Header]|[ Payload[0] ... Payload[N] ]|[Checksum Payload (low)]|[Checksum Payload (high)]|[EndByte]`

h3. CMD

* 2 Byte
** ersten 5 Bit = Device Class[D] (32 mögliche Devices)
** letzten 11 Bit = Kommando ID [X] (2048 Kommandos pro Device Class)
** -> Aufbau: 16 Bit @| DDDDD | XXXXXXXXXXX |@
* einige allgemine Kommandos für alle Geräteklassen
* eigentliche Funktionalität in Kommandos, spezifisch für die jweilige Geräteklasse
** dedizierte Kommandos für jedes Funktion/Modul eines Gerätes, z.B. zwei ADC Kanäle -> kein verschachtelter Aufbau wie z.B. bei XML

h3. Paket Nummer

* fortlaufende Nummer um ACK / Antworten zuordnen zu können
* 0 bis 255
* ggf. ein Zähler pro Geräteklasse (Device ID)
* eher für uns, ob wir damit etwas machen wird sich zeigen

h3. N Bytes

* 2 Bytes -> uint16_t
* gibt die Größe der Payload in Bytes an
** max. 1500 Bytes da der Ethernet Layer maximal 1500 Bytes kann, ggf. wenniger, falls es Einschrängungen auf der MCU gibt....
* redundant, da CMD eindeutig ist und PC und MCU die Größe und Aufbau der Payload kennen müssen
* ggf. einfacher Check, ob die erwartete Payload gesendet wurde

h3. Payload

* N Bytes
* keine feste Zuordnung von Größe oder Inhalt
* Inhalt ist abhängig vom jeweiligen CMD
* Inhalt muss kompakt sein, d.h. es dürfen keine Bytes eingefügt werden, um z.B. die Variablen an der 32 Bit Grenze auszurichten.
** in C/C++ mit `__packed` zu realisieren
** Bespiel:
<pre>
    struct __packed ks2_ComAckError_t {  
        uint16_t     ErrorCode; 
        uint8_t      MSG[KS2_COM__ERROR_MSG_SIZE_MAX];  
    };
</pre>
 
