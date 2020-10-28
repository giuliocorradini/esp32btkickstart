/*
 *  ESP32 Bluetooth Kickstart
 *  main.c
 * 
 *  Author: Giulio Corradini
 *  Date: 2020-10-28
 * 
 */


#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "nvs.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"


#define DEVICE_NAME         "ESP32"
#define SERIAL_PORT_NAME    "MySerial"

/*
 *  Lo stack Bluetooth è composto da diversi livelli impilati di protocolli
 *  che concorrono al funzionamento del dispositivo e alla sua comunicazione
 *  in modalità wireless. Quando usiamo "il Bluetooth" sottointendiamo diversi
 *  protocolli.
 * 
 *  ESP32 gestisce in modo autonomo i layer fino al livello
 *  data-link (protocollo L2CAP).
 *  
 * 
 *  I profili si trovano al livello applicativo
 * 
 */


/*
 *  Profilo SPP - Serial Port Profile
 *  Rende disponibile una porta seriale sul link Bluetooth; come se fosse collegata
 *  fisicamente, possiamo usarla con PuTTY, minicom, screen e qualsiasi software
 *  che possa operare sulle porte seriali.
 * 
 *  La funzione di callback invia un messaggio di benvenuto quando la porta
 *  seriale viene aperta. Per ogni gruppo di byte ricevuto, viene stampato un
 *  messaggio di log con la lunghezza del payload.
 */
char welcome_msg[] = "Hello, world!";
void serial_port_callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *params) {

    //  In base al tipo di evento, compiamo un'adeguata azione
    //  I tipi di event sono descritti nella enum esp_spp_cb_event_t
    switch(event) {
        case ESP_SPP_INIT_EVT:
            esp_spp_start_srv(ESP_SPP_SEC_AUTHENTICATE, ESP_SPP_ROLE_SLAVE, 0, SERIAL_PORT_NAME);
            break;

        case ESP_SPP_SRV_OPEN_EVT:
            //  Qualcuno si è connesso alla nostra porta seriale
            esp_spp_write(params->srv_open.handle, strlen(welcome_msg), (uint8_t*)welcome_msg);
            break;

        case ESP_SPP_DATA_IND_EVT:
            //  Qualcuno ci ha inviato dei dati. Params contiene i dati
            //  arrivati nel campo data_ind.
            printf("Sono arrivati %d bytes dal client %d\n",
                params->data_ind.len,
                params->data_ind.handle         //contiene il numero del client connesso
            );
            ESP_LOG_BUFFER_HEX("Contenuto: ",
                params->data_ind.data,
                params->data_ind.len
            );

            //  echo dei dati
            esp_spp_write(params->data_ind.handle,
                          params->data_ind.len,
                          params->data_ind.data
                        );
            break;

        default:
            //  Tutti gli altri eventi
            printf("Evento del layer SPP: %d\n", event);
    }
}


/*
 *  Profilo GAP - Generic Access Profile
 *  Rende l'ESP visibile agli altri dispositivi Bluetooth e ci permette di
 *  impostare un PIN per collegarci.
 * 
 *  La funzione di callback si occupa di: richiedere il PIN, accettare gli
 *  utilizzatori del Secure Simple Pairing, loggare i connessi.
 */
void gap_layer_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *params) {
    switch(event) {

        //  Qualcuno si vuole collegare con il pin
        case ESP_BT_GAP_PIN_REQ_EVT:
            if(params->pin_req.min_16_digit) {
                printf("Inserisci il codice: 0000 0000 0000 0000\n");
                esp_bt_pin_code_t pin_code = {0};
                esp_bt_gap_pin_reply(params->pin_req.bda, true, 16, pin_code);
            } else {
                printf("Inserisci il codice: 1234\n");
                esp_bt_pin_code_t pin_code = {'1', '2', '3', '4'};
                esp_bt_gap_pin_reply(params->pin_req.bda, true, 4, pin_code);
            }
            break;

        //  Qualcuno vuole collegarsi con il Secure Simple Pairing
        case ESP_BT_GAP_CFM_REQ_EVT:
            esp_bt_gap_ssp_confirm_reply(params->cfm_req.bda, true); //Accettiamo tutti
            break;

        //  È avvenuto un pairing
        case ESP_BT_GAP_AUTH_CMPL_EVT:
            if(params->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
                printf("Il dispositivo %s si è autenticato\n", params->auth_cmpl.device_name);
            } else {
                printf("C'è stato un errore nel pairing. Stato: %d", params->auth_cmpl.stat);
            }
            break;

        default:
            printf("Evento GAP: %d\n", event);
    }
}


void app_main() {
    //  N.B. l'applicazione dovrebbe controllare i codici di ritorno di tutte le funzioni
    //  di configurazione. Questo è un esempio di kickstart e il controllo degli errori
    //  avrebbe ridotto la leggibilità del codice.

    //  Abilitiamo la memoria NVS, un dizionario persistente nella flash
    //  Questa funzione ci permette di ricordare i dispositivi che abbiamo già accoppiato
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    //  Allochiamo la memoria per il Bluetooth e configuriamo il controller Bluetooth
    esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BTDM);

    //  Avviamo lo stack di rete Bluetooth (bluedroid è un fork dello stack bt di Android)
    esp_bluedroid_init();
    esp_bluedroid_enable();
    esp_bt_dev_set_device_name(DEVICE_NAME);

    //  Agganciamo la funzione di callback per il profilo GAP e rendiamo il dispositivo visibile
    esp_bt_gap_register_callback(gap_layer_callback);
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

    //  Agganciamo la funzione di cb per il profilo SPP (porta seriale)
    esp_spp_register_callback(serial_port_callback);
    esp_spp_init(ESP_SPP_MODE_CB);

    // Parametri del Secure Simple Pairing
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
    esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));

    
    // Parametri del pairing classico con codice
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
    esp_bt_pin_code_t pin_code;
    esp_bt_gap_set_pin(pin_type, 0, pin_code);


    //  Lo stack bluedroid crea in automatico un task che inibisce il reboot.
    //  Non abbiamo bisogno di loop infiniti.
}