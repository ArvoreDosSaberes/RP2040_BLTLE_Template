// Tempo, em milissegundos, para o LED piscar rapidamente
// usado para indicar atividade de comunicação BLE (notificações ativas)
#define LED_QUICK_FLASH_DELAY_MS 100

// Tempo, em milissegundos, para o LED piscar lentamente
// usado para indicar estado ocioso ou de espera da conexão BLE
#define LED_SLOW_FLASH_DELAY_MS 1000

// Inicializa a pilha Bluetooth LE do lado cliente.
// Parâmetros:
//  - task: função de callback que será chamada quando uma nova
//          mensagem/notificação BLE for recebida.
//  - message: ponteiro para variável onde o valor recebido (16 bits)
//             será armazenado antes de chamar o callback.
// Retorno:
//  - 0 em caso de sucesso;
//  - valor negativo em caso de falha na inicialização do hardware/BLE.
int bt_client_init(void(*task)(void), uint16_t* message);

// Inicia efetivamente o cliente BLE, ligando o controlador HCI e
// entrando no laço de execução (run loop) da BTstack. Esta função
// bloqueia a execução enquanto a pilha Bluetooth estiver ativa.
void bt_client_start();
