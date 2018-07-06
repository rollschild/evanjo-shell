#include <stdbool.h>
#include <stdio.h>

#define ESH_READ_LINE_BUFFERSIZE 1024

char* EshReadLine(void) {
  int buffersize = ESH_READ_LINE_BUFFERSIZE;
  char* buffer = malloc(buffersize * sizeof(char));
  int position = 0;
  int c;  // Notice this is int, NOT char

  if (!buffer) {
    fprintf(stderr, "esh: allocation error!\n");
    exit(EXIT_FAILURE);
  }

  while (true) {
    c = getchar();

    if (c == EOF || c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
      // ++position;
    }

    ++position;

    if (position >= buffersize) {
      buffersize += ESH_READ_LINE_BUFFERSIZE;
      buffer = realloc(buffer, buffersize);

      if (!buffer) {
        fprintf(stderr, "esh: allocation error!|n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

// Read
// Parse
// Execute
void EshLoop(void) {
  char* line;
  char** args;
  int status;

  do {
    printf("esh=$ ");
    line = EshReadLine();
    args = EshParseLine(line);
    status = EshExecute(args);

    free(line);
    free(args);
  } while (status);

  return;
}

int main(int argc, char** argv) {
  printf("Welcome to the Evanjo shell!\n");
  // Run command loop
  EshLoop();

  return EXIT_SUCCESS;
}
