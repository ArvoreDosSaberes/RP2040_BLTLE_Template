////////////////////////////////////////////////////////////////////////////////
// Raspberry Pi Pico-W Bluetooth Reader Demo
// written by Travis Llado
// last edited 2023-11-08
// Adaptado por Carlos Delfino
// e-mail: consultoria@carlosdelfino.eti.br
// adaptado: 26/11/2025
////////////////////////////////////////////////////////////////////////////////

#include "hardware/pwm.h"
#include "pico/stdlib.h"

#include "log_vt100.h" // Biblioteca de Logging VT100
#include "bt_client_setup.h"  // interface de configuração e inicialização do cliente BLE

////////////////////////////////////////////////////////////////////////////////

// Pino GPIO utilizado para saída PWM.
// No Pico W, o GPIO 21 está ligado ao pino físico correspondente na placa
// (ver diagrama de pinos). Aqui será usado para controlar, por exemplo,
// o brilho de um LED ou a velocidade de um motor.
#define PIN_PWM 21U  // Constante que define o pino GPIO para PWM

// Variável global que armazena o valor de duty cycle recebido via BLE.
// O valor é de 0 a 65535 (16 bits), compatível com a resolução padrão do PWM.
uint16_t _received_duty_;  // Variável global para armazenar o duty cycle recebido

////////////////////////////////////////////////////////////////////////////////

// Função de callback chamada quando um novo valor é recebido via BLE.
// Ela aplica o duty cycle armazenado em `_received_duty_` ao canal PWM
// associado ao pino `PIN_PWM`.
void set_duty(void) {
  // Atualiza o nível de saída do PWM com o novo duty cycle recebido
  pwm_set_gpio_level(PIN_PWM, _received_duty_);
  LOG_DEBUG("Callback set_duty acionado. PWM atualizado para: %u", _received_duty_);
 }

////////////////////////////////////////////////////////////////////////////////

// Configura o hardware de PWM do RP2040 para o pino escolhido.
// Passo a passo:
//  1. Configura o pino GPIO para função PWM.
//  2. Obtém o "slice" de PWM associado a esse pino (cada slice
//     controla um par de saídas PWM).
//  3. Carrega a configuração padrão de PWM.
//  4. Ajusta o divisor de clock (clkdiv) para reduzir a frequência,
//     aumentando a resolução do duty cycle de forma adequada.
//  5. Inicializa o slice com a configuração escolhida e habilita o PWM.
//  6. Garante que o duty cycle inicial seja 0 (saída desligada).
void init_pwm() {
  LOG_INFO("Configurando PWM no pino %d...", PIN_PWM);
  // Configura o pino GPIO para função PWM
  gpio_set_function(PIN_PWM, GPIO_FUNC_PWM);
  // Obtém o "slice" de PWM associado ao pino
  uint slice_num = pwm_gpio_to_slice_num(PIN_PWM);
  // Carrega a configuração padrão de PWM
  pwm_config config = pwm_get_default_config();
  // Ajusta o divisor de clock para reduzir a frequência
  pwm_config_set_clkdiv(&config, 40.0);
  // Inicializa o slice com a configuração escolhida e habilita o PWM
  pwm_init(slice_num, &config, true);
  // Garante que o duty cycle inicial seja 0 (saída desligada)
  pwm_set_gpio_level(PIN_PWM, 0U);
  LOG_DEBUG("PWM inicializado (slice %u, clkdiv 40.0)", slice_num);
}

////////////////////////////////////////////////////////////////////////////////

// Função principal do firmware.
// Fluxo geral do algoritmo:
//  1. Inicializa as rotinas de entrada/saída padrão (UART/USB) com
//     `stdio_init_all`, permitindo uso de `printf` para depuração.
//  2. Configura o módulo de PWM para controlar o pino definido.
//  3. Inicializa o cliente Bluetooth LE, registrando a função de
//     callback `set_duty` e a variável `_received_duty_` como destino
//     dos valores recebidos.
//  4. Inicia a pilha BLE; a partir daqui o código entra no laço
//     interno da BTstack e reage a eventos (notificações do servidor).
//  5. Quando novas notificações chegam, o valor é armazenado em
//     `_received_duty_` e `set_duty` é chamada, atualizando o PWM.
int main() {
    // Inicializa as rotinas de entrada/saída padrão (UART/USB)
    stdio_init_all();

    // Configura nível de log padrão
    log_set_level(LOG_LEVEL_INFO);

    LOG_INFO("Iniciando cliente BLE - Demo Pico W");
    LOG_INFO("Passo 1: Inicializando entrada/saída padrão");

    // Configura o módulo de PWM para controlar o pino definido
    LOG_INFO("Passo 2: Inicializando hardware PWM");
    init_pwm();

    // Inicializa o cliente Bluetooth LE
    LOG_INFO("Passo 3: Inicializando cliente Bluetooth LE (bt_client_init)");
    if (bt_client_init(&set_duty, &_received_duty_) != 0) {
        LOG_WARN("Falha ao inicializar cliente BT!");
        return -1;
    }

    // Inicia a pilha BLE
    LOG_INFO("Passo 4: Iniciando pilha BLE e loop principal (bt_client_start)");
    bt_client_start();

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
