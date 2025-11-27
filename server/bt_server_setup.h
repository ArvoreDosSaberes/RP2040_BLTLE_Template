// Período, em milissegundos, do "heartbeat" do servidor BLE.
// Esse temporizador é utilizado para:
//  - chamar periodicamente a função de callback da aplicação
//    (por exemplo, para nova leitura do ADC);
//  - solicitar à pilha BLE permissão para enviar notificações;
//  - atualizar o estado visual do LED a bordo.
#define HEARTBEAT_PERIOD_MS 100

// Inicializa a pilha Bluetooth LE do lado servidor.
// Parâmetros:
//  - task: função de callback chamada a cada "tick" do heartbeat
//          para atualizar o valor a ser enviado (ex.: leitura ADC);
//  - message: ponteiro para variável de 16 bits que contém o dado
//             a ser notificado ao cliente BLE.
// Retorno:
//  - 0 em caso de sucesso;
//  - valor negativo em caso de falha na inicialização.
int bt_server_init(void(*task)(void), uint16_t* message);

// Inicia efetivamente o servidor BLE, ligando o controlador HCI.
// Depois desta chamada, o dispositivo passa a anunciar (advertising)
// e a responder conexões/notificações conforme configurado.
int bt_server_start();
