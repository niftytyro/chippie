#include "display.h"
#include "interpreter.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  Display display = boot_display();

  if (display.renderer == NULL || display.window == NULL) {
    return 1;
  }

  read_rom_into_memory("../roms/test.ch8");
  execute_rom(display);

  exit_display(display);

  return 0;
}
