#include <stdio.h>
#include <stdlib.h>

void read_file(char path[]) {
  FILE *fptr;

  unsigned char command[2];
  size_t bytes_read =0;

  fptr = fopen(path, "rb");

  while ((bytes_read = fread(command, 1, sizeof(command), fptr))) {
    printf("%x %x\n", command[0], command[1]);
  }

  fclose(fptr);
}

int main() {
  read_file("./test.ch8");
  return 0;
}

