# ESP32 Bluetooth Kickstart

*A cura di Giulio Corradini*

## Configurazione

Clona la cartella del progetto sulla tua macchina e aprila con PlatformIO dal tuo editor.

```
git clone https://github.com/giuliocorradini/esp32_bt_kickstarter
```

Controlla che l'ambiente di sviluppo funzioni e tutte le librerie siano presenti e aggiornate
con il comando Miscellaneous > Update All dalle opezioni di PIO.

Se non riesci a linkare il file oggetto principale con le librerie del bluetooth (esp_bt.h ecc.)
controlla che il supporto al Bluetooth sia abilitato nei file di configurazione del progetto.

```
pio run -t menuconfig
```

Naviga in `Component Config` / `Bluetooth` e abilita l'opzione Bluetooth. Spostandoti nel sottopanello
`Bluetooth Controller` abilita la modalità controller *Dual Mode*.
Abilitiamo anche il profilo SPP in `Bluetooth Options` / `Classic Bluetooth` / `SPP`

Nelle impostazioni di PlatformIO (file platformio.ini) impostiamo il baudrate della porta seriale
dell'ESP32 a 115200 con l'istruzione `monitor_speed = 115200`

## Utilizzo

Cambia il nome del dispositivo, ed eventualmente della porta seriale virtuale, modificando le macro
all'inizio del programma.

