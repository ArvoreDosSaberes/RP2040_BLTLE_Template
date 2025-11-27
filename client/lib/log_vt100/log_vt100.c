#include "log_vt100.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

static log_level_t current_level = LOG_DEFAULT_LEVEL;

static void append_char(char *buf, size_t size, size_t *idx, char c) {
    if (*idx + 1 < size) {
        buf[*idx] = c;
        (*idx)++;
    }
}

static void append_str(char *buf, size_t size, size_t *idx, const char *s) {
    if (!s) s = "(null)";
    while (*s) {
        append_char(buf, size, idx, *s++);
    }
}

static void append_uint(char *buf, size_t size, size_t *idx, unsigned int v) {
    char tmp[16];
    int t = 0;
    if (v == 0) {
        tmp[t++] = '0';
    } else {
        while (v > 0 && t < (int)sizeof(tmp)) {
            tmp[t++] = (char)('0' + (v % 10));
            v /= 10;
        }
    }
    while (t-- > 0) {
        append_char(buf, size, idx, tmp[t]);
    }
}

static void append_int(char *buf, size_t size, size_t *idx, int v) {
    if (v < 0) {
        append_char(buf, size, idx, '-');
        v = -v;
    }
    append_uint(buf, size, idx, (unsigned int)v);
}

static void append_hex(char *buf, size_t size, size_t *idx, unsigned int v, int upper) {
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char tmp[16];
    int t = 0;
    if (v == 0) {
        tmp[t++] = '0';
    } else {
        while (v > 0 && t < (int)sizeof(tmp)) {
            tmp[t++] = digits[v & 0xFU];
            v >>= 4;
        }
    }
    while (t-- > 0) {
        append_char(buf, size, idx, tmp[t]);
    }
}

static void append_binary(char *buf, size_t size, size_t *idx, unsigned int value) {
    int started = 0;
    for (int i = (int)(sizeof(unsigned int) * 8 - 1); i >= 0; --i) {
        unsigned int bit = (value >> i) & 1U;
        if (bit) {
            started = 1;
            append_char(buf, size, idx, '1');
        } else if (started) {
            append_char(buf, size, idx, '0');
        }
    }
    if (!started) {
        append_char(buf, size, idx, '0');
    }
}

static int format_has_binary(const char *fmt) {
    while (*fmt) {
        if (*fmt == '%' && *(fmt + 1) == 'b') {
            return 1;
        }
        ++fmt;
    }
    return 0;
}

// Formatter simplificado com suporte a %b, escrevendo em buffer
static void log_vsnprintf(char *out, size_t size, const char *fmt, va_list ap) {
    size_t idx = 0;

    while (*fmt) {
        if (*fmt != '%') {
            append_char(out, size, &idx, *fmt++);
            continue;
        }

        ++fmt; // skip '%'
        if (*fmt == '%') {
            append_char(out, size, &idx, '%');
            ++fmt;
            continue;
        }

        char spec = *fmt++;
        switch (spec) {
            case 'c': {
                int c = va_arg(ap, int);
                append_char(out, size, &idx, (char)c);
                break;
            }
            case 's': {
                const char *s = va_arg(ap, const char *);
                append_str(out, size, &idx, s);
                break;
            }
            case 'd':
            case 'i': {
                int v = va_arg(ap, int);
                append_int(out, size, &idx, v);
                break;
            }
            case 'u': {
                unsigned int v = va_arg(ap, unsigned int);
                append_uint(out, size, &idx, v);
                break;
            }
            case 'x': {
                unsigned int v = va_arg(ap, unsigned int);
                append_hex(out, size, &idx, v, 0);
                break;
            }
            case 'X': {
                unsigned int v = va_arg(ap, unsigned int);
                append_hex(out, size, &idx, v, 1);
                break;
            }
            case 'p': {
                void *ptr = va_arg(ap, void *);
                unsigned int v = (unsigned int)(uintptr_t)ptr;
                append_str(out, size, &idx, "0x");
                append_hex(out, size, &idx, v, 0);
                break;
            }
            case 'b': {
                unsigned int v = va_arg(ap, unsigned int);
                append_binary(out, size, &idx, v);
                break;
            }
            default:
                // Unknown specifier, print it literally
                append_char(out, size, &idx, '%');
                append_char(out, size, &idx, spec);
                break;
        }
    }

    if (size > 0) {
        out[(idx < size) ? idx : (size - 1)] = '\0';
    }
}

void log_set_level(log_level_t level) {
    current_level = level;
}

void log_write(log_level_t level, const char *fmt, ...) {
    if (level < current_level) {
        return;
    }

    /* Códigos de cor VT100 / ANSI */
    const char *color_reset = "\x1b[0m";
    const char *color_code = "";

    switch (level) {
        case LOG_LEVEL_TRACE: color_code = "\x1b[90m"; break; /* cinza */
        case LOG_LEVEL_DEBUG: color_code = "\x1b[34m"; break; /* azul  */
        case LOG_LEVEL_INFO:  color_code = "\x1b[32m"; break; /* verde */
        case LOG_LEVEL_WARN:  color_code = "\x1b[33m"; break; /* amarelo */
        default:              color_code = "\x1b[0m";  break;
    }

    char msg[256];
    va_list ap;
    va_start(ap, fmt);
    if (format_has_binary(fmt)) {
        log_vsnprintf(msg, sizeof msg, fmt, ap);
    } else {
        vsnprintf(msg, sizeof msg, fmt, ap);
    }
    va_end(ap);

    const char *prefix;
    switch (level) {
        case LOG_LEVEL_TRACE: prefix = "[TRACE] "; break;
        case LOG_LEVEL_DEBUG: prefix = "[DEBUG] "; break;
        case LOG_LEVEL_INFO:  prefix = "[INFO ] "; break;
        case LOG_LEVEL_WARN:  prefix = "[WARN ] "; break;
        default:              prefix = "[LOG  ] "; break;
    }

    printf("%s%s%s%s\n", color_code, prefix, msg, color_reset);
}

