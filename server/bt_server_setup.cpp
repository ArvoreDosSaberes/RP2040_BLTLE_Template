////////////////////////////////////////////////////////////////////////////////

#include "btstack.h"
#include "pico/cyw43_arch.h"
#include "pico/btstack_cyw43.h"
#include "temp_sensor.h"
#include "pico.h"
#include "log_vt100.h"
#include "bt_server_setup.h"

////////////////////////////////////////////////////////////////////////////////

// Flags de advertising BLE (0x06 = LE General Discoverable Mode
// + BR/EDR not supported), conforme especificação Bluetooth.
#define APP_AD_FLAGS 0x06

////////////////////////////////////////////////////////////////////////////////

// Estrutura de timer usada como "heartbeat" periódico da aplicação.
btstack_timer_source_t heartbeat;
// Registro para callback de eventos HCI (estado da pilha, conexões, etc.).
btstack_packet_callback_registration_t hci_event_callback_registration;

// Flag que indica se o cliente habilitou notificações na característica.
int le_notification_enabled;
// Handle da conexão atual com o cliente BLE.
hci_con_handle_t con_handle;

// Ponteiro global para a função de callback fornecida pela aplicação.
// Tipicamente, esta função atualiza o valor da variável exposta via GATT
// (por exemplo, leitura de ADC).
void(*global_callback_task)(void);
// Ponteiro para a variável de 16 bits cujo conteúdo será enviado
// nas notificações GATT ao cliente.
uint16_t* global_callback_message;

// Dados de advertising (anúncio) BLE:
//  - Flags gerais;
//  - Nome completo do dispositivo ("Pico 00:00:00:00:00:00");
//  - Lista de serviços de 16 bits (aqui, Environmental Sensing 0x181A).
uint8_t adv_data[] = {
    // Flags general discoverable
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, APP_AD_FLAGS,
    // Name
    0x17, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'P', 'i', 'c', 'o', ' ', '0', '0', ':', '0', '0', ':', '0', '0', ':', '0', '0', ':', '0', '0', ':', '0', '0',
    0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, 0x1a, 0x18,
};
const uint8_t adv_data_len = sizeof(adv_data);

////////////////////////////////////////////////////////////////////////////////

// Callbacks ATT e funções de controle do servidor BLE.
uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size);
int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
int bt_server_init(void(*task)(void), uint16_t* message);
int bt_server_start();
void heartbeat_handler(struct btstack_timer_source *ts);
void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

////////////////////////////////////////////////////////////////////////////////

// Callback de leitura ATT.
// Quando o cliente faz uma leitura direta da característica de
// temperatura, este callback é chamado para fornecer o valor atual.
uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size) {
    UNUSED(connection_handle);

    if (att_handle == ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_TEMPERATURE_01_VALUE_HANDLE){
        // Retorna o conteúdo da variável apontada por
        // `global_callback_message` para o cliente.
        LOG_DEBUG("ATT Read Callback: Enviando valor atual (%d) para o cliente", *global_callback_message);
        return att_read_callback_handle_blob((const uint8_t *)&global_callback_message, sizeof(global_callback_message), offset, buffer, buffer_size);
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

// Callback de escrita ATT.
// Usado aqui para tratar escritas no Client Characteristic Configuration
// Descriptor (CCCD), que habilitam ou desabilitam notificações.
int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    UNUSED(transaction_mode);
    UNUSED(offset);
    UNUSED(buffer_size);
    
    if (att_handle != ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_TEMPERATURE_01_CLIENT_CONFIGURATION_HANDLE) return 0;
    // Interpreta o valor escrito pelo cliente: se igual a
    // `GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION`,
    // significa que o cliente deseja receber notificações.
    le_notification_enabled = little_endian_read_16(buffer, 0) == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;
    con_handle = connection_handle;
    
    if (le_notification_enabled) {
        LOG_INFO("Notificações ativadas pelo cliente (Handle: 0x%04X)", con_handle);
        // Solicita à pilha ATT a geração de um evento
        // `ATT_EVENT_CAN_SEND_NOW`, no qual será enviada
        // a próxima notificação.
        att_server_request_can_send_now_event(con_handle);
    } else {
        LOG_INFO("Notificações desativadas pelo cliente");
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

// Inicializa o servidor BLE:
//  - armazena o callback da aplicação e o ponteiro para a variável
//    de dados que será exposta via GATT;
//  - inicializa o driver CYW43 (Wi-Fi/Bluetooth do Pico W);
//  - inicializa L2CAP, Security Manager (SM) e o servidor ATT com
//    a tabela de atributos `profile_data` gerada a partir do .gatt;
//  - registra os handlers de eventos HCI e ATT;
//  - configura e agenda o timer de heartbeat.
int bt_server_init(void(*task)(void), uint16_t* message) {
    global_callback_task = task;
    global_callback_message = message;

    // initialize CYW43 driver architecture (will enable BT if/because CYW43_ENABLE_BLUETOOTH == 1)
    if (cyw43_arch_init()) {
        printf("failed to initialise cyw43_arch\n");
        return -1;
    }

    // Inicializa o restante da pilha BTstack.
    l2cap_init();
    sm_init();
    att_server_init(profile_data, att_read_callback, att_write_callback);

    // Registra callback para ser informado sobre mudanças de estado
    // da BTstack (ex.: quando entra em HCI_STATE_WORKING).
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // Registra o handler para eventos ATT (incluindo CAN_SEND_NOW).
    att_server_register_packet_handler(packet_handler);

    // Configura o timer de heartbeat para disparar periodicamente,
    // chamando `heartbeat_handler` e atualizando o LED/dados.
    heartbeat.process = &heartbeat_handler;
    btstack_run_loop_set_timer(&heartbeat, HEARTBEAT_PERIOD_MS);
    btstack_run_loop_add_timer(&heartbeat);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////

// Liga o controlador HCI. Depois desta chamada, o dispositivo
// passa a anunciar e aceitar conexões BLE.
int bt_server_start() {
    // turn on bluetooth!
    LOG_INFO("Ligando controlador HCI (hci_power_control)...");
    hci_power_control(HCI_POWER_ON);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////

// Handler do timer de heartbeat.
// Responsável por:
//  - chamar o callback da aplicação para atualizar o valor exposto;
//  - solicitar permissão para enviar notificações, se habilitadas;
//  - piscar o LED a bordo como indicação visual de atividade.
void heartbeat_handler(struct btstack_timer_source *ts) {
    static uint32_t counter = 0;
    counter++;

    // Atualiza os dados de aplicação (ex.: nova leitura ADC).
    global_callback_task();
    // Opcional: LOG_TRACE("Heartbeat #%u - Valor atual: %d", counter, *global_callback_message);
    LOG_INFO("Heartbeat #%u - Valor atual: %d", counter, *global_callback_message);
    if (le_notification_enabled) {
        att_server_request_can_send_now_event(con_handle);
    }

    // Inverte o estado do LED on-board.
    static int led_on = true;
    led_on = !led_on;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);

    // Reinicia o timer para o próximo "tick" do heartbeat.
    btstack_run_loop_set_timer(ts, HEARTBEAT_PERIOD_MS);
    btstack_run_loop_add_timer(ts);
}

////////////////////////////////////////////////////////////////////////////////

// Handler principal de pacotes HCI/ATT.
// Trata:
//  - entrada da pilha em estado operacional (configuração de advertising);
//  - fim de conexões (limpando flag de notificação);
//  - eventos CAN_SEND_NOW para efetivamente enviar notificações.
void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(size);
    UNUSED(channel);
    bd_addr_t local_addr;
    if (packet_type != HCI_EVENT_PACKET) return;

    uint8_t event_type = hci_event_packet_get_type(packet);
    switch(event_type){
        case BTSTACK_EVENT_STATE:{
            if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
            gap_local_bd_addr(local_addr);
            printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));

            // Configura os parâmetros de advertising (intervalo, tipo,
            // endereço) e registra o bloco de dados `adv_data`.
            uint16_t adv_int_min = 800;
            uint16_t adv_int_max = 800;
            uint8_t adv_type = 0;
            bd_addr_t null_addr;
            memset(null_addr, 0, 6);
            gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
            assert(adv_data_len <= 31); // ble limitation
            gap_advertisements_set_data(adv_data_len, (uint8_t*) adv_data);
            gap_advertisements_enable(1);

            global_callback_task();

            break;}
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            // Ao desconectar, desabilita o envio de notificações.
            le_notification_enabled = 0;
            break;
        case ATT_EVENT_CAN_SEND_NOW:
            // Momento em que a pilha garante que podemos enviar um
            // pacote de notificação. Enviamos o conteúdo da variável
            // apontada por `global_callback_message` ao cliente.
            att_server_notify(con_handle, ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_TEMPERATURE_01_VALUE_HANDLE, (uint8_t*)global_callback_message, sizeof(global_callback_message));
            printf("wrote %d\n", *global_callback_message);
            break;
        default:
            break;
    }
}
