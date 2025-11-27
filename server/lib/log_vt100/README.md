# log_vt100

![Visitantes do Projeto](https://visitor-badge.laobi.icu/badge?page_id=arvoredossaberes.log_vt100)
[![License: CC BY 4.0](https://img.shields.io/badge/License-CC%20BY%204.0-lightgrey.svg)](LICENSE)
![C](https://img.shields.io/badge/C-ANSI-blue)
![Pico SDK](https://img.shields.io/badge/Raspberry%20Pi-Pico%20SDK-brightgreen)
![CMake](https://img.shields.io/badge/CMake-%3E%3D3.16-informational)

Biblioteca de **log colorido em terminal VT100/ANSI** para firmwares em C, com suporte a níveis de log (TRACE, DEBUG, INFO, WARN) e formato estendido com `%b` para exibir valores binários. Foi pensada para uso em ambientes como **Raspberry Pi Pico** (via `pico_stdlib`), mas pode ser reaproveitada em qualquer projeto C que escreva em `stdout`.

## Visão geral

- **Objetivo:** simplificar o uso de logs estruturados em projetos embarcados, com cores por nível e filtragem em tempo de compilação.
- **Saída:** mensagens com prefixos (`[INFO]`, `[DEBUG]`, etc.) e códigos de cor VT100/ANSI.
- **Formato:** compatível com `printf`, incluindo `%d`, `%u`, `%x`, `%p`, `%s`, `%c` e o extra `%b` (binário).

## Arquivos principais

- `log_vt100.h` – API pública (tipos, macros de nível e configuração).
- `log_vt100.c` – implementação do formatador e escrita em `printf`.

## API

### Tipos

```c
typedef enum {
    LOG_LEVEL_TRACE = 0,
    LOG_LEVEL_DEBUG = 1,
    LOG_LEVEL_INFO  = 2,
    LOG_LEVEL_WARN  = 3,
} log_level_t;
```

### Funções

```c
void log_set_level(log_level_t level);
void log_write(log_level_t level, const char *fmt, ...);
```

- `log_set_level` permite alterar o nível de log **em tempo de execução**.
- `log_write` é a função base usada pelos macros (`LOG_TRACE`, `LOG_DEBUG`, etc.).

### Macros de uso

```c
LOG_TRACE("valor=%d", v);
LOG_DEBUG("Inicializando modulo");
LOG_INFO("Estado: %u", status);
LOG_WARN("Falha: codigo=%d", err);
```

## Configuração em tempo de compilação

A configuração é feita via `#define` **antes** de incluir `log_vt100.h` (em geral em um header global do projeto):

```c
// Nível padrão (se log_set_level nao for chamado)
#define LOG_DEFAULT_LEVEL LOG_LEVEL_INFO

// Verbosidade de compilacao
// -1: nenhum log; 0: apenas WARN;
//  1: INFO+WARN; 2: DEBUG+; 3: TRACE+
#define LOG_LEVEL 2

// Opcional: tag do modulo (na versao atual, reservado para extensoes futuras)
#define LOG_TAG "MAIN"

#include "log_vt100.h"
```

- **`LOG_LEVEL` < 0**: desliga todos os logs (útil para builds de producao sensiveis a desempenho).
- **`LOG_LEVEL` >= 0**: `LOG_WARN` sempre ativo; demais níveis dependem do valor de `LOG_LEVEL`.

## Cores VT100/ANSI

A biblioteca usa os seguintes códigos de cor:

- `TRACE` – cinza (`\x1b[90m`)
- `DEBUG` – azul (`\x1b[34m`)
- `INFO`  – verde (`\x1b[32m`)
- `WARN`  – amarelo (`\x1b[33m`)

Cada linha termina com `\x1b[0m` para resetar o estilo do terminal.

## Suporte ao formato binario (`%b`)

`log_vt100` adiciona o especificador `%b` para imprimir inteiros sem sinal em binario:

```c
uint32_t flags = 0b101100;
LOG_DEBUG("flags em binario: %b", flags);
// Saida aproximada: [DEBUG] flags em binario: 101100
```

Quando o formatador detecta `%b`, ele usa um caminho de formatacao proprio, mantendo os demais especificadores compatíveis com `printf`.

## Integração com CMake / Pico SDK

Exemplo de integracao (conforme `CMakeLists.txt` desta lib):

```cmake
add_library(log_vt100 STATIC
    log_vt100.c
)

target_include_directories(log_vt100 PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(log_vt100
    pico_stdlib
)
```

Em um projeto que usa esta biblioteca:

```cmake
add_executable(meu_firmware
    main.c
    # outros fontes
)

target_link_libraries(meu_firmware
    pico_stdlib
    log_vt100
)
```

E em `main.c`:

```c
#include "pico/stdlib.h"
#define LOG_LEVEL 2
#include "log_vt100.h"

int main() {
    stdio_init_all();

    LOG_INFO("Sistema inicializado");

    uint32_t mask = 0x0F;
    LOG_DEBUG("mask=0x%X (%b)", mask, mask);

    while (true) {
        LOG_TRACE("loop");
        sleep_ms(1000);
    }
}
```

## Uso dentro do workspace Keyboard-Menu

Quando usada junto ao workspace `Keyboard-Menu---workspace`, esta biblioteca pode:

- Exibir **logs estruturados** de tarefas FreeRTOS (teclado, menu, OLED, etc.).
- Mostrar **mascaras de bits** (por exemplo, de grupos de eventos) em formato binario com `%b`.
- Facilitar o debug de estados internos sem depender de depurador.

Basta garantir que o executável do exemplo esteja linkado com `log_vt100` e incluir `log_vt100.h` nos módulos onde você deseja logar.

## Licença

Consulte `LICENSE` no repositório para detalhes de uso e redistribuição.
