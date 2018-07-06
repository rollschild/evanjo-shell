#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ESH_READ_LINE_BUFFERSIZE 1024
#define ESH_TOKENS_BUFFERSIZE 64
#define ESH_TOKEN_DELIM " \t\n\r\a"

char** EshParseLine(char* line) {
  int buffersize = ESH_TOKENS_BUFFERSIZE;
  int position = 0;
  char** tokens = malloc(buffersize * sizeof(char*));
  char* token;

  if (!tokens) {
    fprintf(stderr, "esh: allocation error!\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, ESH_TOKEN_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    ++position;

    if (position >= buffersize) {
      buffersize += ESH_TOKENS_BUFFERSIZE;
      tokens = realloc(tokens, buffersize * sizeof(char*));

      if (!tokens) {
        fprintf(stderr, "esh: allocation error!\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, ESH_TOKEN_DELIM);
  }

  tokens[position] = NULL;
  return tokens;
}

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
      buffer = realloc(buffer, buffersize * sizeof(char));

      if (!buffer) {
        fprintf(stderr, "esh: allocation error!|n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

// Alternative way to EshReadLine()
/*
char* EshReadLine(void) {
  char* line = NULL;
  ssize_t buffersize = 0;
  getline(&line, &buffersize, stdin);
  return line;
}
*/
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
