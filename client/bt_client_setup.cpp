#include "btstack.h"
#include "pico/cyw43_arch.h"

#include "log_vt100.h"
#include "bt_client_setup.h"

// Máquina de estados do cliente GATT ("Temperature Client").
// Cada valor representa uma fase do ciclo de vida da conexão BLE:
//  - TC_OFF: pilha/desconectado, sem operação ativa;
//  - TC_IDLE: reservado para estado ocioso (não utilizado neste exemplo);
//  - TC_W4_SCAN_RESULT: aguardando resultados de varredura (scan) de anúncios;
//  - TC_W4_CONNECT: aguardando conclusão da tentativa de conexão LE;
//  - TC_W4_SERVICE_RESULT: aguardando resultado da descoberta de serviço GATT;
//  - TC_W4_CHARACTERISTIC_RESULT: aguardando descoberta de característica;
//  - TC_W4_ENABLE_NOTIFICATIONS_COMPLETE: aguardando conclusão da escrita
//    da configuração de notificação na característica (Client Characteristic Configuration);
//  - TC_W4_READY: pronto para receber notificações do servidor.
typedef enum {
    TC_OFF,
    TC_IDLE,
    TC_W4_SCAN_RESULT,
    TC_W4_CONNECT,
    TC_W4_SERVICE_RESULT,
    TC_W4_CHARACTERISTIC_RESULT,
    TC_W4_ENABLE_NOTIFICATIONS_COMPLETE,
    TC_W4_READY
} gc_state_t;

// Registro para callback de eventos HCI (BTstack).
static btstack_packet_callback_registration_t hci_event_callback_registration;
// Estado atual da máquina de estados do cliente.
static gc_state_t state = TC_OFF;
// Endereço Bluetooth LE do servidor (dispositivo remoto) encontrado.
static bd_addr_t server_addr;
// Tipo de endereço (público, random, etc.).
static bd_addr_type_t server_addr_type;
// Handle da conexão HCI ativa.
static hci_con_handle_t connection_handle;
// Estrutura que representa o serviço GATT descoberto no servidor.
static gatt_client_service_t server_service;
// Estrutura que representa a característica GATT utilizada para receber dados.
static gatt_client_characteristic_t server_characteristic;
// Flag que indica se o listener de notificações está atualmente registrado.
static bool listener_registered;
// Estrutura de listener de notificações GATT (registro na BTstack).
static gatt_client_notification_t notification_listener;
// Timer periódico usado como "heartbeat" para piscar o LED indicando estado.
static btstack_timer_source_t heartbeat;

// Ponteiro global para função de callback fornecida pela aplicação.
// Esta função será chamada sempre que uma nova notificação GATT chegar.
void(*global_callback_task)(void);
// Ponteiro global para a variável onde o valor recebido (16 bits) será gravado.
uint16_t* global_callback_message;

// Inicia o processo de "scan" BLE em busca de um servidor com o
// serviço esperado (Environmental Sensing). É chamada quando a
// pilha Bluetooth entra em estado de funcionamento (HCI_STATE_WORKING)
// ou após uma desconexão, para tentar reconectar.
static void client_start(void){
    // Inicia o processo de scan BLE.
    LOG_INFO("Iniciando Scan BLE (gap_start_scan)...");
    state = TC_W4_SCAN_RESULT;
    gap_set_scan_parameters(0,0x0030, 0x0030);
    gap_start_scan();
}

// Varre o conteúdo de um relatório de anúncio (advertising report)
// para verificar se o dispositivo remoto anuncia o UUID de serviço
// desejado (16 bits). Retorna true se encontrar o serviço.
static bool advertisement_report_contains_service(uint16_t service, uint8_t *advertisement_report){
    // get advertisement from report event
    const uint8_t * adv_data = gap_event_advertising_report_get_data(advertisement_report);
    uint8_t adv_len  = gap_event_advertising_report_get_data_length(advertisement_report);

    // iterate over advertisement data
    ad_context_t context;
    for (ad_iterator_init(&context, adv_len, adv_data) ; ad_iterator_has_more(&context) ; ad_iterator_next(&context)){
        uint8_t data_type = ad_iterator_get_data_type(&context);
        uint8_t data_size = ad_iterator_get_data_len(&context);
        const uint8_t * data = ad_iterator_get_data(&context);
        switch (data_type){
            case BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS:
                for (int i = 0; i < data_size; i += 2) {
                    uint16_t type = little_endian_read_16(data, i);
                    if (type == service) return true;
                }
            default:
                break;
        }
    }
    return false;
}

// Callback de eventos do cliente GATT.
// Responsável por:
//  - tratar o resultado da descoberta de serviços;
//  - tratar o resultado da descoberta de características;
//  - registrar o listener de notificações e habilitar notificações;
//  - receber e repassar notificações GATT para a aplicação.
static void handle_gatt_client_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(packet_type);
    UNUSED(channel);
    UNUSED(size);

    uint8_t att_status;
    switch(state){
        case TC_W4_SERVICE_RESULT:
            // Nesta fase, estamos aguardando a resposta da descoberta de serviços.
            switch(hci_event_packet_get_type(packet)) {
                case GATT_EVENT_SERVICE_QUERY_RESULT:
                    // store service (we expect only one)
                    LOG_INFO("Serviço encontrado. Armazenando...");
                    gatt_event_service_query_result_get_service(packet, &server_service);
                    break;
                case GATT_EVENT_QUERY_COMPLETE:
                    att_status = gatt_event_query_complete_get_att_status(packet);
                    if (att_status != ATT_ERROR_SUCCESS){
                        LOG_WARN("Falha na descoberta de serviço. ATT Error 0x%02x", att_status);
                        gap_disconnect(connection_handle);
                        break;  
                    } 
                    // Descoberta de serviço concluída com sucesso;
                    // agora passamos para a descoberta da característica
                    // específica (por UUID) dentro desse serviço.
                    state = TC_W4_CHARACTERISTIC_RESULT;
                    LOG_INFO("Serviço descoberto. Buscando característica Environmental Sensing...");
                    gatt_client_discover_characteristics_for_service_by_uuid16(handle_gatt_client_event, connection_handle, &server_service, ORG_BLUETOOTH_CHARACTERISTIC_TEMPERATURE);
                    break;
                default:
                    break;
            }
            break;
        case TC_W4_CHARACTERISTIC_RESULT:
            // Nesta fase, aguardamos o resultado da descoberta da
            // característica desejada dentro do serviço encontrado.
            switch(hci_event_packet_get_type(packet)) {
                case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT:
                    LOG_INFO("Característica encontrada. Armazenando...");
                    gatt_event_characteristic_query_result_get_characteristic(packet, &server_characteristic);
                    break;
                case GATT_EVENT_QUERY_COMPLETE:
                    LOG_INFO("Descoberta de característica concluída. Status: 0x%02x", gatt_event_query_complete_get_att_status(packet));
                    att_status = gatt_event_query_complete_get_att_status(packet);
                    if (att_status != ATT_ERROR_SUCCESS){
                        LOG_WARN("Falha na descoberta de característica. ATT Error 0x%02x", att_status);
                        gap_disconnect(connection_handle);
                        break;  
                    } 
                    // Registro do handler que receberá futuras
                    // notificações de valor dessa característica.
                    listener_registered = true;
                    gatt_client_listen_for_characteristic_value_updates(&notification_listener, handle_gatt_client_event, connection_handle, &server_characteristic);
                    // Habilita notificações na característica escrevendo
                    // na Client Characteristic Configuration Descriptor.
                    LOG_INFO("Característica encontrada. Habilitando notificações (Write CCCD)...");
                    state = TC_W4_ENABLE_NOTIFICATIONS_COMPLETE;
                    gatt_client_write_client_characteristic_configuration(handle_gatt_client_event, connection_handle,
                        &server_characteristic, GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION);
                    break;
                default:
                    break;
            }
            break;
        case TC_W4_ENABLE_NOTIFICATIONS_COMPLETE:
            // Aguarda a confirmação da escrita na configuração de
            // notificações da característica.
            switch(hci_event_packet_get_type(packet)) {
                case GATT_EVENT_QUERY_COMPLETE:
                    att_status = gatt_event_query_complete_get_att_status(packet);
                    LOG_INFO("Notificações habilitadas, status ATT: 0x%02x", att_status);
                    if (att_status != ATT_ERROR_SUCCESS) break;
                    state = TC_W4_READY;
                    LOG_INFO("CLIENTE PRONTO! Aguardando notificações do servidor...");
                    break;
                default:
                    break;
            }
            break;
        case TC_W4_READY:
            // Estado READY: já conectado, serviço/characteristic
            // conhecidos, notificações habilitadas. Agora recebemos
            // efetivamente as notificações com dados do servidor.
            switch(hci_event_packet_get_type(packet)) {
                case GATT_EVENT_NOTIFICATION: {
                    uint16_t value_length = gatt_event_notification_get_value_length(packet);
                    const uint8_t *value = gatt_event_notification_get_value(packet);
                    LOG_INFO("Notificação recebida (len: %d)", value_length);
                    // Neste exemplo, espera-se que a notificação tenha
                    // 4 bytes; utilizamos os 2 primeiros como valor de
                    // 16 bits em little endian, compatível com a
                    // variável `global_callback_message`.
                    if (value_length == 4) {    // apparently messages are 4bytes?
                        *global_callback_message = little_endian_read_16(value, 0);
                        // Chama a função de callback da aplicação para
                        // reagir ao novo valor recebido.
                        global_callback_task();
                        LOG_INFO("Valor lido: %d", *global_callback_message);
                    } else {
                        LOG_WARN("Comprimento inesperado: %d", value_length);
                    }
                    break;
                }
                default:
                    LOG_WARN("Packet type desconhecido no estado READY: 0x%02x", hci_event_packet_get_type(packet));
                    break;
            }
            break;
        default:
            LOG_WARN("Estado desconhecido na máquina de estados GATT");
            break;
    }
}

// Handler de eventos HCI genéricos (nível GAP/HCI).
// Ele coordena o início do scan, a conexão com o servidor, o
// tratamento da conclusão da conexão e a reação à desconexão.
static void hci_event_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(size);
    UNUSED(channel);
    bd_addr_t local_addr;
    if (packet_type != HCI_EVENT_PACKET) return;

    uint8_t event_type = hci_event_packet_get_type(packet);
    switch(event_type){
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
                gap_local_bd_addr(local_addr);
                LOG_INFO("BTstack operacional no endereço %s", bd_addr_to_str(local_addr));
                // Quando a pilha está pronta, iniciamos o processo de scan.
                client_start();
            } else {
                state = TC_OFF;
            }
            break;
        case GAP_EVENT_ADVERTISING_REPORT:
            if (state != TC_W4_SCAN_RESULT) return;
            // Verifica se o anúncio contém o serviço desejado.
            if (!advertisement_report_contains_service(ORG_BLUETOOTH_SERVICE_ENVIRONMENTAL_SENSING, packet)) return;
            // store address and type
            gap_event_advertising_report_get_address(packet, server_addr);
            server_addr_type = static_cast<bd_addr_type_t>(gap_event_advertising_report_get_address_type(packet));
            // Para o scan e tenta conectar ao dispositivo encontrado.
            state = TC_W4_CONNECT;
            gap_stop_scan();
            LOG_INFO("Servidor encontrado! Conectando a %s...", bd_addr_to_str(server_addr));
            gap_connect(server_addr, server_addr_type);
            break;
        case HCI_EVENT_LE_META:
            // wait for connection complete
            switch (hci_event_le_meta_get_subevent_code(packet)) {
                case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                    if (state != TC_W4_CONNECT) return;
                    connection_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                    // Conexão LE estabelecida, iniciamos a descoberta
                    // do serviço primário de Environmental Sensing.
                    LOG_INFO("Conectado! Iniciando descoberta de serviços (Environmental Sensing)...");
                    state = TC_W4_SERVICE_RESULT;
                    gatt_client_discover_primary_services_by_uuid16(handle_gatt_client_event, connection_handle, ORG_BLUETOOTH_SERVICE_ENVIRONMENTAL_SENSING);
                    break;
                default:
                    break;
            }
            break;
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            // unregister listener
            connection_handle = HCI_CON_HANDLE_INVALID;
            if (listener_registered){
                listener_registered = false;
                gatt_client_stop_listening_for_characteristic_value_updates(&notification_listener);
            }
            LOG_INFO("Desconectado de %s", bd_addr_to_str(server_addr));
            if (state == TC_OFF) break;
            // Se não foi um desligamento definitivo, tenta reiniciar o
            // processo de scan para reconectar automaticamente.
            client_start();
            break;
        default:
            break;
    }
}

// Função periódica chamada pelo loop da BTstack para atualizar o
// estado visual do LED a bordo (CYW43_WL_GPIO_LED_PIN).
// Comportamento:
//  - pisca lentamente quando não há listener registrado;
//  - alterna entre pulsos rápidos quando há notificações ativas,
//    servindo como indicação visual do estado da conexão BLE.
static void heartbeat_handler(struct btstack_timer_source *ts) {
    // Invert the led
    static bool quick_flash;
    static bool led_on = true;

    led_on = !led_on;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
    if (listener_registered && led_on) {
        quick_flash = !quick_flash;
    } else if (!listener_registered) {
        quick_flash = false;
    }

    // Restart timer
    btstack_run_loop_set_timer(ts, (led_on || quick_flash) ? LED_QUICK_FLASH_DELAY_MS : LED_SLOW_FLASH_DELAY_MS);
    btstack_run_loop_add_timer(ts);
}

// Inicializa o cliente BLE:
//  - armazena o callback da aplicação e a variável de mensagem;
//  - inicializa o driver CYW43 (Wi-Fi/Bluetooth do Pico W);
//  - configura a L2CAP, Security Manager (SM) e servidor ATT vazio;
//  - inicializa o cliente GATT;
//  - registra o handler de eventos HCI;
//  - configura e agenda o timer de heartbeat para o LED.
int bt_client_init(void(*task)(void), uint16_t* message) {
    global_callback_task = task;
    global_callback_message = message;

    // initialize CYW43 driver architecture (will enable BT if/because CYW43_ENABLE_BLUETOOTH == 1)
    if (cyw43_arch_init()) {
        LOG_WARN("Falha ao inicializar cyw43_arch");
        return -1;
    }

    LOG_DEBUG("cyw43_arch_init() sucesso");

    l2cap_init();
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);

    // setup empty ATT server - only needed if LE Peripheral does ATT queries on its own, e.g. Android and iOS
    att_server_init(NULL, NULL, NULL);

    gatt_client_init();
    LOG_DEBUG("L2CAP, SM, ATT Server e GATT Client inicializados");

    hci_event_callback_registration.callback = &hci_event_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // set one-shot btstack timer
    heartbeat.process = &heartbeat_handler;
    btstack_run_loop_set_timer(&heartbeat, LED_SLOW_FLASH_DELAY_MS);
    btstack_run_loop_add_timer(&heartbeat);

    return 0;
}

// Liga o controlador HCI e entra no laço principal da BTstack.
// A partir desta chamada, o fluxo passa a ser dirigido por eventos
// (callbacks de HCI/GATT) até que a pilha seja desligada.
void bt_client_start() {
    // turn on!
    LOG_INFO("Ligando controlador HCI (hci_power_control)...");
    hci_power_control(HCI_POWER_ON);

    LOG_INFO("Entrando no loop de execução da BTstack (btstack_run_loop_execute)");
    btstack_run_loop_execute();
}

