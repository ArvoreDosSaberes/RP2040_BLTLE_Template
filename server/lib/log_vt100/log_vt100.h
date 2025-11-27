#ifndef LOG_H
#define LOG_H

#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Níveis de severidade para mensagens de log.
// Quanto menor o valor numérico, mais "verbo" é o nível:
//  - TRACE: mensagens muito detalhadas, para depuração fina.
//  - DEBUG: informações de depuração em geral.
//  - INFO: mensagens informativas de alto nível.
//  - WARN: avisos sobre condições inesperadas, mas não fatais.
typedef enum {
    LOG_LEVEL_TRACE = 0,
    LOG_LEVEL_DEBUG = 1,
    LOG_LEVEL_INFO  = 2,
    LOG_LEVEL_WARN  = 3,
} log_level_t;

// Define o nível mínimo de log que será exibido em tempo de execução.
// Mensagens abaixo desse nível são descartadas por `log_write`.
void log_set_level(log_level_t level);

// Função principal de escrita de log.
// Parâmetros:
//  - level: nível da mensagem (TRACE/DEBUG/INFO/WARN).
//  - fmt: string de formato no estilo printf, podendo incluir `%b`
//         para impressão em binário (ver implementação em log_vt100.c).
//  - ...: argumentos variádicos correspondentes aos especificadores
//         da string de formato.
void log_write(log_level_t level, const char *fmt, ...);

// Nível padrão de log utilizado para inicializar o sistema de logging
// caso nenhum outro seja configurado em tempo de execução.
#ifndef LOG_DEFAULT_LEVEL
#define LOG_DEFAULT_LEVEL LOG_LEVEL_INFO
#endif

// Nível de log em tempo de compilação.
// Controla quais macros LOG_* serão efetivamente compiladas:
//  - -1: nenhum log gerado (todas as macros viram NOP).
//  -  0: apenas WARN é mantido.
//  -  1: INFO e WARN são mantidos.
//  -  2: DEBUG, INFO e WARN são mantidos.
//  -  3: TRACE + todos os demais níveis são mantidos.
#ifndef LOG_LEVEL
// -1: nenhum log; 0: apenas WARN; 1: INFO+WARN; 2: DEBUG+; 3: TRACE+
#define LOG_LEVEL 1
#endif

// TAG opcional associada ao módulo/arquivo.
// Pode ser usada futuramente para prefixar as mensagens com o nome
// do subsistema; atualmente não é utilizada na formatação.
#ifndef LOG_TAG
#define LOG_TAG NULL
#endif

// Macro genérica que mapeia para a função `log_write`, convertendo
// o nível simbólico (TRACE, DEBUG, etc.) para o valor do enum.
// Exemplo: LOG(INFO, "valor=%d", x);
#define LOG(level, fmt, ...) \
    log_write(LOG_LEVEL_##level, fmt, ##__VA_ARGS__)

// Mapeamento de verbosidade em tempo de compilação.
// Dependendo de LOG_LEVEL, algumas macros abaixo viram NOP (não geram
// código), reduzindo o tamanho do firmware e o overhead de log.
#if LOG_LEVEL < 0

#define LOG_TRACE(fmt, ...) ((void)0)
#define LOG_DEBUG(fmt, ...) ((void)0)
#define LOG_INFO(fmt, ...)  ((void)0)
#define LOG_WARN(fmt, ...)  ((void)0)

#else

#if LOG_LEVEL >= 3
#define LOG_TRACE(fmt, ...) LOG(TRACE, fmt, ##__VA_ARGS__)
#else
#define LOG_TRACE(fmt, ...) ((void)0)
#endif

#if LOG_LEVEL >= 2
#define LOG_DEBUG(fmt, ...) LOG(DEBUG, fmt, ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...) ((void)0)
#endif

#if LOG_LEVEL >= 1
#define LOG_INFO(fmt, ...)  LOG(INFO,  fmt, ##__VA_ARGS__)
#else
#define LOG_INFO(fmt, ...)  ((void)0)
#endif

// WARN fica sempre ativo quando LOG_LEVEL >= 0
#define LOG_WARN(fmt, ...)  LOG(WARN,  fmt, ##__VA_ARGS__)

#endif // LOG_LEVEL < 0

#ifdef __cplusplus
}
#endif

#endif // LOG_H

