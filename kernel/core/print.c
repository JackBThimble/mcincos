#include "print.h"
#include <stdint.h>

static char hex_digit(unsigned v) {
    return (v < 10) ? ('0' + v) : ('a' + (v - 10));
}

static void print_uint(uint64_t v, int base, int width, char pad, int upper) {
    char buf[32];
    int i = 0;

    if (v == 0) {
        buf[i++] = '0';
    } else {
        while (v) {
            uint64_t d = v % base;
            buf[i++] = (d < 10) ? '0' + d : (upper ? 'A' : 'a') + (d - 10);
            v /= base;
        }
    }

    while (i < width) {
        kputc(pad);
        width--;
    }

    while (i--) {
        kputc(buf[i]);
    }
}

static void print_int(int64_t v, int base) {
    if (v < 0) {
        kputc('-');
        print_uint((uint8_t)-v, base, 0, ' ', 0);
    } else {
        print_uint((uint64_t)v, base, 0, ' ', 0);
    }
}

void kvprintf(const char *fmt, va_list args) {
    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            kputc(*fmt);
            continue;
        }

        fmt++;

        int width = 0;
        char pad = ' ';

        if (*fmt == '0') {
            pad = 0;
            fmt++;
        }

        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }

        switch (*fmt) {
        case 'c':
            kputc((char)va_arg(args, int));
            break;

        case 's': {
            const char *s = va_arg(args, const char *);
            if (!s)
                s = "(null)";
            while (*s)
                kputc(*s++);
            break;
        }

        case 'd':
        case 'i':
            print_int(va_arg(args, int), 10);
            break;

        case 'u':
            print_uint(va_arg(args, unsigned), 10, width, pad, 0);
            break;

        case 'x':
            print_uint(va_arg(args, unsigned), 16, width, pad, 0);
            break;

        case 'X':
            print_uint(va_arg(args, unsigned), 16, width, pad, 1);
            break;

        case 'p':
            kputs("0x");
            print_uint((uint64_t)va_arg(args, void *), 16, 16, '0', 0);
            break;

        case 'l':
            if (*(fmt + 1) == 'l') {
                fmt++;
                if (*(fmt + 1) == 'x') {
                    fmt++;
                    print_uint(va_arg(args, uint64_t), 16, width, pad, 0);
                } else if (*(fmt + 1) == 'u') {
                    fmt++;
                    print_uint(va_arg(args, uint64_t), 10, width, pad, 0);
                }
            }
            break;

        case '%':
            kputc('%');
            break;

        default:
            kputc('%');
            kputc(*fmt);
            break;
        }
    }
}

void kprint_hex_u64(uint64_t v) {
    kprint("0x");
    for (int i = 15; i >= 0; i--) {
        unsigned nib = (unsigned)((v >> (i * 4)) & 0xf);
        char c[2] = {hex_digit(nib), 0};
        kprint(c);
    }
}

void kputs(const char *s) {
    while (*s)
        kputc(*s++);
}

void kprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    kvprintf(fmt, args);
    va_end(args);
}

void kputc(char c) {
    __asm__ volatile(".intel_syntax noprefix\n"
                     "out dx, al\n"
                     ".att_syntax prefix\n"
                     :
                     : "a"(c), "d"(0x3f8));
}

void kprint(const char *s) {
    while (*s) {
        kputc((uint8_t)*s++);
    }
}

void kprintln(const char *s) {
    kprint(s);
    kputc('\n');
}

void kprintlnf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    kvprintf(fmt, args);
    va_end(args);
    kputc('\n');
}
