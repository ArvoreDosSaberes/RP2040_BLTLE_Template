# Pico W – Demo de Sensor/Leitor via Bluetooth

![visitors](https://visitor-badge.laobi.icu/badge?page_id=ArvoreDosSaberes.RP2040_BLTLE_Template)
[![Build](https://img.shields.io/github/actions/workflow/status/ArvoreDosSaberes/RP2040_BLTLE_Template/ci.yml?branch=main)](https://github.com/ArvoreDosSaberes/RP2040_BLTLE_Template/actions)
[![Issues](https://img.shields.io/github/issues/ArvoreDosSaberes/RP2040_BLTLE_Template)](https://github.com/ArvoreDosSaberes/RP2040_BLTLE_Template/issues)
[![Stars](https://img.shields.io/github/stars/ArvoreDosSaberes/RP2040_BLTLE_Template)](https://github.com/ArvoreDosSaberes/RP2040_BLTLE_Template/stargazers)
[![Forks](https://img.shields.io/github/forks/ArvoreDosSaberes/RP2040_BLTLE_Template)](https://github.com/ArvoreDosSaberes/RP2040_BLTLE_Template/network/members)
[![Language](https://img.shields.io/badge/Language-C%2FC%2B%2B-brightgreen.svg)]()
[![License: CC BY 4.0](https://img.shields.io/badge/license-CC%20BY%204.0-blue.svg)](https://creativecommons.org/licenses/by/4.0/)

Projeto simples, copiado do Pico SDK e modificado para demonstrar funcionalidades de **Bluetooth Low Energy (BLE)** usando duas placas **Raspberry Pi Pico W**:

- **Server**: lê uma entrada analógica (tensão) no GPIO 26 / canal ADC 0 e transmite o valor via BLE.
- **Client**: recebe o valor via BLE e gera um **PWM** proporcional no GPIO 21.

É possível verificar o funcionamento tanto pela saída serial USB de cada placa quanto ligando dispositivos analógicos:

- **Entrada analógica** (ex.: potenciômetro) conectada ao `writer`.
- **Saída analógica/atuador** (ex.: driver de motor, LED de alta potência, etc.) conectada ao `reader`.

Assim, você pode controlar um dispositivo conectado ao `reader` girando o potenciômetro ligado ao `writer`.

---

## Estrutura do projeto

Dentro do diretório `RP2040_BLTLE_Template/` você encontrará:

- `client/`

  - `client.cpp`: código principal que lê o ADC no GPIO 26 e envia o valor via BLE.
  - `bt_client_setup.cpp` / `bt_setup.h`: configuração de Bluetooth e callbacks.
  - `temp_sensor.gatt`: definição do serviço/característica BLE usada para enviar os dados.
  - `CMakeLists.txt`: configuração de build para o executável `writer`.
- `server/`

  - `server.cpp`: código principal que recebe o valor via BLE e ajusta o PWM no GPIO 21.
  - `bt_server_setup.cpp` / `bt_setup.h`: configuração de Bluetooth e callbacks.
  - `CMakeLists.txt`: configuração de build para o executável `reader`.

---

## Requisitos

- 2 × **Raspberry Pi Pico W**.
- SDK oficial do Raspberry Pi Pico instalado e configurado.
- Toolchain C/C++ com CMake (versão mínima 3.12, conforme `CMakeLists.txt`).
- Cabo micro‑USB para cada Pico W.
- Opcional, mas recomendado:
  - **Potenciômetro** para usar como entrada analógica no `writer`.
  - **Carga/driver** (ex.: driver de motor, LED + resistor, etc.) para saída PWM no `reader`.

---

## Como compilar

1. **Configurar o SDK do Pico** conforme a documentação oficial.
2. No diretório raiz do template (`RP2040_BLTLE_Template/`), crie um diretório de build para cada projeto ou um build compartilhado:

   ```bash
   mkdir -p build-server
   cd build-server
   cmake ../server
   make
   ```

   Isso irá gerar o binário/UF2 do `writer` (por exemplo, `writer.uf2`).
3. Para o `reader`:

   ```bash
   mkdir -p ../build-reader
   cd ../build-reader
   cmake ../reader
   make
   ```

   Isso irá gerar o binário/UF2 do `reader` (por exemplo, `reader.uf2`).

---

## Como gravar o firmware em cada Pico W

1. **Coloque o Pico W em modo BOOTSEL**:

   - Segure o botão **BOOTSEL** na placa.
   - Conecte o cabo USB ao computador.
   - Solte o botão.
   - O Pico aparecerá como um drive USB (ex.: `RPI-RP2`).
2. **Gravar o `writer`**:

   - Copie o arquivo `writer.uf2` (gerado em `build-writer/`) para o drive `RPI-RP2`.
   - O Pico irá reiniciar automaticamente com o firmware do `writer`.
3. **Gravar o `reader`**:

   - Repita o processo usando a segunda placa Pico W.
   - Copie o arquivo `reader.uf2` (gerado em `build-reader/`) para o `RPI-RP2` da segunda placa.

---

## Tutorial de uso: par writer/reader

### 1. Conexões de hardware

- **Placa `writer`**:

  - Conecte um **potenciômetro** ao **GPIO 26** (canal ADC 0).
    - Pino 1 do potenciômetro → 3V3.
    - Pino 2 (cursor) → GPIO 26.
    - Pino 3 → GND.
- **Placa `reader`**:

  - Conecte sua carga ao **GPIO 21** (saída PWM):
    - Por exemplo, um **driver de motor** ou um **LED + resistor**.
    - Verifique sempre as correntes máximas permitidas pelo Pico W. Use estágios de potência/driver quando necessário.

### 2. Funcionamento em alto nível

1. O `writer` inicializa o ADC no GPIO 26 e o stack BLE.
2. Periodicamente, o `writer` lê o valor do ADC e atualiza a característica BLE.
3. O `reader` se conecta ao serviço BLE exposto pelo `writer`.
4. Sempre que um novo valor é recebido, o `reader` ajusta o nível de PWM no GPIO 21.
5. O resultado é que o atuador conectado ao `reader` responde diretamente ao valor lido no `writer`.

### 3. Monitorando via USB Serial

Ambos os projetos habilitam **stdio via USB**, então você pode:

- Abrir um terminal serial (ex.: `minicom`, `screen`, `picocom` ou o monitor serial da sua IDE) na porta correspondente a cada Pico W.
- Usar a taxa de baud configurada por padrão no SDK (tipicamente 115200 bps).
- Acompanhar mensagens de debug (se habilitadas nos códigos `bt_setup.cpp` ou similares).

---

## Sobre o Bluetooth (BLE)

O projeto utiliza o **Bluetooth stack do Pico W (BTStack)** com:

- Um **servidor BLE** rodando no `writer`, expondo uma característica de valor analógico (originalmente temperatura, adaptada para leitura de tensão/ADC).
- Um **cliente BLE** no `reader`, que se conecta ao serviço do `writer` e lê as atualizações de valor.

Os detalhes de serviços, características e callbacks estão encapsulados nos arquivos `bt_setup.cpp` e `bt_setup.h` de cada projeto.

---

## Licença

Este projeto está licenciado sob **Creative Commons Attribution 4.0 International (CC BY 4.0)**.

Você é livre para:

- Compartilhar — copiar e redistribuir o material em qualquer suporte ou formato.
- Adaptar — remixar, transformar e criar a partir deste material para qualquer fim, inclusive comercial.

Desde que:

- Dê o devido crédito, fornecendo um link para a licença e indicando se foram feitas modificações.
- Não imponha restrições adicionais que impeçam outros de exercerem os mesmos direitos concedidos pela licença.

Consulte o texto completo da licença em:

- https://creativecommons.org/licenses/by/4.0/

Um arquivo separado de licença também é fornecido neste repositório.

---

## Créditos

- Baseado em exemplo do **Raspberry Pi Pico SDK**.
- Adaptações e organização do template por **Carlos Delfino**.
- Código original do demo Bluetooth (`writer`/`reader`) por **Travis Llado**.
