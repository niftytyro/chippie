#include <stdio.h>
#include <stdlib.h>

void read_rom(char path[]) {
  FILE *fptr;

  unsigned char command[2];
  size_t bytes_read = 0;

  fptr = fopen(path, "rb");

  while ((bytes_read = fread(command, 1, sizeof(command), fptr))) {
    printf("%x%x ", command[1], command[0]);
  }

  fclose(fptr);
}

int main() {
  read_rom("./test.ch8");
  return 0;
}
