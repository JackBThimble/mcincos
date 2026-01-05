#include "arch.h"
#include "../core/panic.h"

void arch_fatal(const char *msg) { panic(msg); }
