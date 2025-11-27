# Pico W – Reader (Atuador PWM BLE)

![visitors](https://visitor-badge.laobi.icu/badge?page_id=ArvoreDosSaberes.RP2040_BLTLE_Template)
[![Issues](https://img.shields.io/github/issues/ArvoreDosSaberes/RP2040_BLTLE_Template)](https://github.com/ArvoreDosSaberes/RP2040_BLTLE_Template/issues)
[![Stars](https://img.shields.io/github/stars/ArvoreDosSaberes/RP2040_BLTLE_Template)](https://github.com/ArvoreDosSaberes/RP2040_BLTLE_Template/stargazers)
[![Forks](https://img.shields.io/github/forks/ArvoreDosSaberes/RP2040_BLTLE_Template)](https://github.com/ArvoreDosSaberes/RP2040_BLTLE_Template/network/members)
[![Language](https://img.shields.io/badge/Language-C%2FC%2B%2B-brightgreen.svg)]()
[![License: CC BY 4.0](https://img.shields.io/badge/license-CC%20BY%204.0-blue.svg)](https://creativecommons.org/licenses/by/4.0/)

Subprojeto **reader** do template `RP2040_BLTLE_Template` para Raspberry Pi Pico W.

Este firmware atua como **cliente BLE**: conecta-se ao serviço exposto pelo `writer`, recebe o valor enviado (ex.: leitura de ADC) e ajusta um **PWM** no **GPIO 21**, permitindo controlar um atuador conectado a este pino.

---

## Arquivos principais

- `reader.cpp`: ponto de entrada do firmware, inicializa o PWM no GPIO 21 e registra callbacks BLE para atualizar o duty cycle com o valor recebido.
- `bt_setup.cpp` / `bt_setup.h`: configuração do stack Bluetooth (BTStack) e lógica de conexão/leitura das características do `writer`.
- `CMakeLists.txt`: configuração de build para gerar o executável/UF2 `reader`.

---

## Como compilar

A partir do diretório raiz do template (`RP2040_BLTLE_Template/`):

```bash
mkdir -p build-reader
cd build-reader
cmake ../reader
make
```

Isso irá gerar o binário/UF2 do `reader` (por exemplo, `reader.uf2`).

---

## Como gravar no Pico W

1. Coloque o segundo Pico W em modo **BOOTSEL**:
   - Segure o botão **BOOTSEL**.
   - Conecte o cabo USB ao computador.
   - Solte o botão.
   - O Pico aparecerá como um drive USB (ex.: `RPI-RP2`).

2. Copie o arquivo `reader.uf2` (da pasta `build-reader/`) para o drive `RPI-RP2`.
3. O Pico irá reiniciar automaticamente com o firmware `reader`.

---

## Uso típico

- Conecte sua carga ao **GPIO 21** (saída PWM):
  - Por exemplo, um **driver de motor**, **LED + resistor**, ou outro estágio de potência adequado.
  - Respeite as limitações de corrente do Pico W, utilizando drivers externos quando necessário.
- Quando o `reader` estiver conectado ao `writer` via BLE, ele ajustará o PWM no GPIO 21 conforme o valor recebido (tipicamente lido de um potenciômetro no `writer`).

---

## Monitorando via USB Serial

O projeto habilita **stdio via USB**. Você pode abrir um terminal serial na porta do Pico W para acompanhar mensagens de debug (quando presentes).

---

## Licença

Este subprojeto segue a mesma licença do repositório principal: **Creative Commons Attribution 4.0 International (CC BY 4.0)**.

Para mais detalhes, consulte o README na raiz do repositório ou acesse:

- https://creativecommons.org/licenses/by/4.0/
