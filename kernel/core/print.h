#pragma once
#include <stdarg.h>
#include <stdint.h>

void kputc(char c);
void kputs(const char *s);

void kprintf(const char *fmt, ...);
void kvprintf(const char *fmt, va_list args);
void kprintlnf(const char *fmt, ...);

void kprint(const char *s);
void kprintln(const char *s);

void kprint_hex_u64(uint64_t v);
