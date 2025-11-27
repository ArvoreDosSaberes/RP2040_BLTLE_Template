////////////////////////////////////////////////////////////////////////////////
// Raspberry Pi Pico-W Bluetooth Writer Demo
// written by Travis Llado
// last edited 2023-11-07
// Adaptado por Carlos Delfino
// e-mail: consultoria@carlosdelfino.eti.br
// adaptado: 26/11/2025
////////////////////////////////////////////////////////////////////////////////

#include "hardware/adc.h"
#include "pico/stdlib.h"

#include "log_vt100.h"

#include "bt_server_setup.h"  // interface de configuração e inicialização do servidor BLE

////////////////////////////////////////////////////////////////////////////////

// Canal GPIO utilizado como entrada analógica.
// No Pico W, o GPIO 26 corresponde ao canal ADC0 do conversor analógico-digital.
#define PIN_26_GPIO_CHANNEL 26U

// Canal interno do ADC associado ao GPIO 26.
#define PIN_26_ADC_CHANNEL  0U

// Variável global que armazena a última leitura do ADC (16 bits).
// Este valor será enviado periodicamente via BLE para o cliente.
uint16_t _adc_reading_;

////////////////////////////////////////////////////////////////////////////////

// Função de callback chamada periodicamente pelo código BLE.
// Responsável por realizar uma nova leitura do ADC e atualizar
// a variável global `_adc_reading_` com o valor lido.
void read_adc(void) {
    // Seleciona o canal ADC correspondente ao pino configurado.
    adc_select_input(PIN_26_GPIO_CHANNEL);
    // Realiza a conversão analógica-digital e armazena o resultado.
    _adc_reading_ = adc_read();
 }

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

// Função principal do firmware do servidor BLE.
// Fluxo geral do algoritmo:
//  1. Inicializa as rotinas de entrada/saída padrão (UART/USB) com
//     `stdio_init_all` para permitir uso de `printf`.
//  2. Inicializa o módulo ADC do RP2040, configura o pino GPIO 26
//     como entrada analógica e seleciona o canal ADC correspondente.
//  3. Opcionalmente habilita o sensor de temperatura interno
//     (`adc_set_temp_sensor_enabled`), dependendo do uso desejado.
//  4. Inicializa o servidor Bluetooth LE com `bt_init`, registrando
//     a função de callback `read_adc` e a variável `_adc_reading_`
//     como fonte de dados a serem enviados.
//  5. Inicia a pilha BLE com `bt_start`; a partir deste ponto, o
//     envio de notificações e a leitura periódica do ADC passam a
//     ser controlados pelos handlers internos em `bt_server_setup.cpp`.
//  6. Entra em um laço infinito apenas para manter o programa ativo;
//     toda a lógica de BLE e de leitura de ADC ocorre via callbacks.
int main() {
    // Inicializa as rotinas de entrada/saída padrão (UART/USB)
    stdio_init_all();

    // Configura nível de log padrão
    log_set_level(LOG_LEVEL_INFO);

    LOG_INFO("Iniciando servidor BLE - Demo Pico W");
    LOG_INFO("Passo 1: Inicializando entrada/saída padrão (stdio_init_all)");

    // Configura o módulo ADC e o pino de entrada analógica
    LOG_INFO("Passo 2: Configurando ADC e GPIO %d", PIN_26_GPIO_CHANNEL);
    adc_init();
    adc_gpio_init(PIN_26_GPIO_CHANNEL);
    adc_select_input(PIN_26_ADC_CHANNEL);
    adc_set_temp_sensor_enabled(true);
    LOG_DEBUG("ADC inicializado, sensor de temperatura ativado");

    // Inicializa o servidor Bluetooth LE
    LOG_INFO("Passo 3: Inicializando servidor Bluetooth LE (bt_server_init)");
    if (bt_server_init(&read_adc, &_adc_reading_) != 0) {
        LOG_WARN("Falha ao inicializar servidor BT!");
        return -1;
    }
    
    // Inicia a pilha BLE
    LOG_INFO("Passo 4: Iniciando pilha BLE (bt_server_start)");
    bt_server_start();
    
    LOG_INFO("Passo 5: Entrando no loop principal infinito");
    while(true) {   
        // Loop principal ocioso: toda a atividade fica
        // a cargo dos handlers internos da pilha BLE.
        sleep_ms(1000);
        // Opcional: log periódico de "heartbeat" do main loop em nível debug
        // LOG_DEBUG("Loop principal ativo...");
    }
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
