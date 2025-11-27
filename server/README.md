# Pico W – Writer (Sensor BLE)

[![Visitors](https://komarev.com/ghpvc/?username=ArvoreDosSaberes&repo=RP2040_BLTLE_Template-writer&color=blue&style=flat)](https://github.com/ArvoreDosSaberes/RP2040_BLTLE_Template/tree/main/writer)
[![Issues](https://img.shields.io/github/issues/ArvoreDosSaberes/RP2040_BLTLE_Template)](https://github.com/ArvoreDosSaberes/RP2040_BLTLE_Template/issues)
[![Stars](https://img.shields.io/github/stars/ArvoreDosSaberes/RP2040_BLTLE_Template)](https://github.com/ArvoreDosSaberes/RP2040_BLTLE_Template/stargazers)
[![Forks](https://img.shields.io/github/forks/ArvoreDosSaberes/RP2040_BLTLE_Template)](https://github.com/ArvoreDosSaberes/RP2040_BLTLE_Template/network/members)
[![Language](https://img.shields.io/badge/Language-C%2FC%2B%2B-brightgreen.svg)]()
[![License: CC BY 4.0](https://img.shields.io/badge/license-CC%20BY%204.0-blue.svg)](https://creativecommons.org/licenses/by/4.0/)

Subprojeto **writer** do template `RP2040_BLTLE_Template` para Raspberry Pi Pico W.

Este firmware lê uma entrada analógica (tensão) no **GPIO 26 / ADC 0** e transmite o valor via **Bluetooth Low Energy (BLE)**. Ele atua como **servidor BLE**, expondo uma característica com o valor lido do ADC.

---

## Arquivos principais

- `writer.cpp`: ponto de entrada do firmware, inicializa o ADC e a pilha BLE, e registra o callback de leitura.
- `bt_setup.cpp` / `bt_setup.h`: configuração do stack Bluetooth (BTStack), serviços, características e callbacks de notificação.
- `temp_sensor.gatt`: definição do serviço/característica BLE usada para enviar os dados do ADC.
- `CMakeLists.txt`: configuração de build para gerar o executável/UF2 `writer`.

---

## Como compilar

A partir do diretório raiz do template (`RP2040_BLTLE_Template/`):

```bash
mkdir -p build-writer
cd build-writer
cmake ../writer
make
```

Isso irá gerar o binário/UF2 do `writer` (por exemplo, `writer.uf2`).

---

## Como gravar no Pico W

1. Coloque o Pico W em modo **BOOTSEL**:
   - Segure o botão **BOOTSEL**.
   - Conecte o cabo USB ao computador.
   - Solte o botão.
   - O Pico aparecerá como um drive USB (ex.: `RPI-RP2`).

2. Copie o arquivo `writer.uf2` (da pasta `build-writer/`) para o drive `RPI-RP2`.
3. O Pico irá reiniciar automaticamente com o firmware `writer`.

---

## Uso típico

- Conecte um **potenciômetro** ao **GPIO 26**:
  - Pino 1 → 3V3.
  - Pino 2 (cursor) → GPIO 26.
  - Pino 3 → GND.
- O firmware lerá o valor analógico (0–3,3 V, conforme limites do ADC) e o enviará continuamente via BLE.
- Normalmente, este writer é pareado com o subprojeto **`reader`**, que recebe o valor e gera um PWM proporcional em outro Pico W.

---

## Monitorando via USB Serial

O projeto habilita **stdio via USB**. Você pode abrir um terminal serial (ex.: `minicom`, `screen`, `picocom` ou monitor serial da IDE) na porta do Pico W para visualizar mensagens de debug.

---

## Licença

Este subprojeto segue a mesma licença do repositório principal: **Creative Commons Attribution 4.0 International (CC BY 4.0)**.

Para mais detalhes, consulte o README na raiz do repositório ou acesse:

- https://creativecommons.org/licenses/by/4.0/
