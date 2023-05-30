#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "display.h"

void read_rom_into_memory(char path[]);

void execute_rom(Display display);

#endif
